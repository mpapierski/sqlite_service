#if !defined(SQLITE_SERVICE_SERVICE_HPP_)
#define SQLITE_SERVICE_SERVICE_HPP_

#include <string>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#include <sqlite3.h>

#include "sqlite_service/detail/error.hpp"

namespace services { namespace sqlite {

class database
{
public:
	database(boost::asio::io_service & io_service)
		: io_service_(io_service)
		, processing_thread_(boost::bind(&boost::asio::io_service::run, &processing_service_))
	{
	}
	~database()
	{
		processing_service_.stop();
		processing_thread_.join();
	}
	template <typename HandlerT>
	void async_open(const ::std::string & url, HandlerT callback)
	{
		begin_work();
		processing_service_.post(boost::bind(&database::__async_open<HandlerT>, this, url, callback));
	}
private:
	inline void begin_work()
	{
		assert(!work_ && "Already working!");
		work_.reset(new boost::asio::io_service::work(io_service_));
	}
	inline void end_work()
	{
		work_.reset();
	}
	/**
	 * Open database connection in blocking mode.
	 */
	template <typename HandlerT>
	void __async_open(const ::std::string & url, HandlerT handler)
	{
		boost::system::error_code ec;
		int result;
		struct sqlite3 * conn = NULL;
		if ((result = sqlite3_open(url.c_str(), &conn)) != SQLITE_OK)
		{
			ec.assign(result, get_error_category());
		}
		conn_.reset(conn, &sqlite3_close);
		io_service_.post(boost::bind(handler, ec));
		end_work();
	}
	/** Results of blocking methods are posted here */
	boost::asio::io_service & io_service_;
	/** All blocking methods gets posted here */
	boost::asio::io_service processing_service_;
	boost::thread processing_thread_;
	boost::shared_ptr<struct sqlite3> conn_;
	/** Service is busy (thread is doing something) */
	boost::scoped_ptr<boost::asio::io_service::work> work_;
};

} }

#endif