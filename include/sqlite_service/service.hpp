#if !defined(SQLITE_SERVICE_SERVICE_HPP_)
#define SQLITE_SERVICE_SERVICE_HPP_

#include <string>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
#include <sqlite3.h>

#include "sqlite_service/detail/error.hpp"

namespace services { namespace sqlite {

class database
{
public:
	database(boost::asio::io_service & io_service)
		: io_service_(io_service)
		, processing_work_(processing_service_)
		, processing_thread_(boost::bind(&database::run_wrapper, this))
	{
	}
	~database()
	{
		processing_service_.stop();
		processing_thread_.join();
	}
	void run_wrapper()
	{
		processing_service_.run();
	}
	/**
	 * Open database connection asynchronous.
	 * @param url URL parameter.
	 * @param handler Callback which will be fired after open is done.
	 */
	template <typename OpenHandler>
	void async_open(const ::std::string & url, OpenHandler handler)
	{
		processing_service_.post(boost::bind(
			&database::async_open_task<boost::_bi::protected_bind_t<OpenHandler> >,
			this,
			url,
			boost::make_shared<boost::asio::io_service::work>(boost::ref(io_service_)),
			boost::protect(handler)));
	}
	/**
	 * Execute query. For each row in the result passed handler will be called.
	 * @param query Query
	 * @param handler Handler to be called for each row.
	 */
	template <typename EachHandler>
	void async_fetch(const ::std::string & query, EachHandler handler)
	{
		processing_service_.post(boost::bind(
			&database::async_fetch_task<boost::_bi::protected_bind_t<EachHandler> >,
			this,
			query,
			boost::make_shared<boost::asio::io_service::work>(boost::ref(io_service_)),
			boost::protect(handler)));
	}
	/**
	 * Execute query. Run callback after statement was executed.
	 * @param query Query
	 * @param handler Handler to be called after query was executed.
	 */
	template <typename ExecHandler>
	void async_exec(const ::std::string & query, ExecHandler handler)
	{
		processing_service_.post(boost::bind(
			&database::async_exec_task<boost::_bi::protected_bind_t<ExecHandler> >,
			this,
			query,
			boost::make_shared<boost::asio::io_service::work>(boost::ref(io_service_)),
			boost::protect(handler)));
	}
	/**
	 * Non throwing version of blocking database open.
	 * @param url URL address of database.
	 * @param ec Error code 
	 */
	void open(const ::std::string & url, boost::system::error_code & ec)
	{
		int result;
		struct sqlite3 * conn = NULL;
		if ((result = sqlite3_open(url.c_str(), &conn)) != SQLITE_OK)
		{
			ec.assign(result, get_error_category());
		}
		// Connection pointer could be NULL after open.
		if (!conn)
		{
			ec.assign(SQLITE_NOMEM, get_error_category());
		}
		conn_.reset(conn, &sqlite3_close);
	}
	/**
	 * Throwing version fo blocking database open.
	 * @param url URL address of database.
	 */
	void open(const ::std::string & url)
	{
		boost::system::error_code ec;
		open(url, ec);
		throw_database_error(ec);
	}
private:
	/**
	 * Open database connection in blocking mode.
	 */
	template <typename HandlerT>
	void async_open_task(const ::std::string & url, boost::shared_ptr<boost::asio::io_service::work> work, HandlerT handler)
	{
		boost::system::error_code ec;
		open(url, ec);
		io_service_.post(boost::bind(handler, ec));
	}
	/**
	 * This structure holds required temporary data needed by sqlite3_exec.
	 */
	template <typename T>
	struct baton
	{
		database * self;
		T handler;
		baton(database * _self, T _handler)
			: self(_self)
			, handler(_handler)
		{
		}
	};
	/**
	 * Execute query in blocking mode
	 * @param query Query
	 * @param work Work instance used to keep primary io service alive.
	 * @param handler Handler callback called once when there is an error
	 * or for each row in the result.
	 */
	template <typename HandlerT>
	void async_fetch_task(const ::std::string & query, boost::shared_ptr<boost::asio::io_service::work> work, HandlerT handler)
	{
		boost::scoped_ptr<baton<HandlerT> > btn(new baton<HandlerT>(this, handler));
		int result = sqlite3_exec(conn_.get(), query.c_str(), &exec_callback<HandlerT>, btn.get(), NULL);
		if (result == SQLITE_ERROR || result == SQLITE_MISUSE)
		{
			// Construct error object holding details
			boost::system::error_code ec;
			ec.assign(result, get_error_category());
			io_service_.post(boost::bind(handler, ec));
		}
		else if (result == SQLITE_BUSY)
		{
			assert(false && "Unsupported");
		}
	}
	template <typename HandlerT>
	void async_exec_task(const ::std::string & query, boost::shared_ptr<boost::asio::io_service::work> work, HandlerT handler)
	{
		boost::system::error_code ec;
		int result;
		result = sqlite3_exec(conn_.get(), query.c_str(), NULL, NULL, NULL);
		if (result == SQLITE_BUSY)
		{
			assert(false && "Unsupported"); // TODO: Implement reexec transparent to the callee.
		}
		else if (result == SQLITE_ERROR || result == SQLITE_MISUSE)
		{
			ec.assign(result, get_error_category());
		}
		io_service_.post(boost::bind(handler, ec));
	}
	template <typename HandlerT>
	static int exec_callback(void * data, int columns, char ** values, char ** column_names)
	{
		assert(data);
		baton<HandlerT> * btn = static_cast<baton<HandlerT> *>(data);
		assert(btn->self);
		boost::system::error_code ec;
		btn->self->io_service_.post(boost::bind(btn->handler, ec));
		return 0;
	}
	/**
	 * Throws exception with detailed SQLite error.
	 * @param ec Error code
	 */
	inline void throw_database_error(const boost::system::error_code & ec) const
	{
		if (ec)
		{
			throw boost::system::system_error(ec, ::sqlite3_errmsg(conn_.get()));
		}
	}
	/** Results of blocking methods are posted here */
	boost::asio::io_service & io_service_;
	/** All blocking methods gets posted here */
	boost::asio::io_service processing_service_;
	/** Keep processing thread always busy */
	boost::asio::io_service::work processing_work_;
	/** This thread runs processing queue */
	boost::thread processing_thread_;
	/** Shared instance of sqlite3 connection */
	boost::shared_ptr<struct sqlite3> conn_;
};

} }

#endif