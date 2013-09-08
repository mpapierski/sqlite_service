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
		, processing_thread_(boost::bind(&boost::asio::io_service::run, &processing_service_))
	{
	}
	~database()
	{
		processing_service_.stop();
		processing_thread_.join();
	}

	template <typename OpenHandler>
	void async_open(const ::std::string & url, OpenHandler handler)
	{
		processing_service_.post(boost::bind(&database::__async_open<boost::_bi::protected_bind_t<OpenHandler> >, this,
			url,
			boost::make_shared<boost::asio::io_service::work>(boost::ref(io_service_)),
			boost::protect(handler)));
	}
private:
	/**
	 * Open database connection in blocking mode.
	 */
	template <typename HandlerT>
	void __async_open(std::string url, boost::shared_ptr<boost::asio::io_service::work> work, HandlerT handler)
	{
		boost::system::error_code ec;
		int result;
		struct sqlite3 * conn = NULL;
		if ((result = sqlite3_open(url.c_str(), &conn)) != SQLITE_OK)
		{
			ec.assign(result, get_error_category());
		}
		if (!conn)
		{
			ec.assign(SQLITE_NOMEM, get_error_category());
		}
		conn_.reset(conn, &sqlite3_close);
		io_service_.post(boost::bind(handler, ec));
	}
	/** Results of blocking methods are posted here */
	boost::asio::io_service & io_service_;
	/** All blocking methods gets posted here */
	boost::asio::io_service processing_service_;
	boost::thread processing_thread_;
	boost::shared_ptr<struct sqlite3> conn_;
};

} }

#endif