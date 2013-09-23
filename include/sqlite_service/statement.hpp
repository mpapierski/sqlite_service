#if !defined(SQLITE_SERVICE_STATEMENT_HPP_)
#define SQLITE_SERVICE_STATEMENT_HPP_

#include <boost/move/move.hpp>

#include <boost/tuple/tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/include/boost_tuple.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility.hpp>
#include <boost/ref.hpp>
#include "sqlite3.h"

#include "aux/assign_columns.hpp"
#include "aux/bind_params.hpp"

namespace services { namespace sqlite {

class service;

/**
 * Statement wrapper
 */
class statement
{
private:
	inline static void safe_sqlite3_finalize(struct sqlite3_stmt * stmt)
	{
		assert(stmt && "Statement is NULL");
		int result = sqlite3_finalize(stmt);
		assert(result == SQLITE_OK && "Statement is not finalized.");
	}
public:
	statement(boost::asio::io_service & io_svc)
		: io_service_(io_svc)
	{
	}
	statement(boost::asio::io_service & io_svc, boost::shared_ptr<struct sqlite3> conn, const ::std::string & query)
		: io_service_(io_svc)
		, conn_(conn)
	{
		assert(conn_ && "NULL connection!");
		int result;
		struct sqlite3_stmt * stmt = NULL;
		if ((result = sqlite3_prepare_v2(conn_.get(), query.c_str(), -1, &stmt, NULL)) != SQLITE_OK)
		{
			ec_.assign(result, get_error_category());
			last_error_ = ::sqlite3_errmsg(conn_.get());
			return;
		}
		assert(stmt && "Statement is not prepared.");
		stmt_.reset(stmt, &safe_sqlite3_finalize);
	}
	int step() const
	{
		assert(stmt_ && "Statement is NULL");
		return sqlite3_step(stmt_.get());
	}
	template <typename TupleType>
	bool fetch(TupleType & results)
	{
		int result = step();
		if (result == SQLITE_ROW)
		{
			int index = 0;
			boost::fusion::for_each(results, aux::assign_columns(stmt_, index));
		}
		return result == SQLITE_ROW;
	}
	template <typename TupleType>
	void bind_params(TupleType bind_args)
	{
		int index = 1;
		boost::fusion::for_each(bind_args, aux::bind_params(stmt_, index));
	}
	inline const ::std::string & last_error() const
	{
		return last_error_;
	}
	boost::system::error_code & error()
	{
		return ec_;
	}
	template <typename ResultT, typename HandlerT>
	void async_fetch(HandlerT handler) const
	{
		int index = 0;
		int result = SQLITE_MISUSE;
		boost::system::error_code ec;
		ResultT row;
		while (stmt_ && (result = step()) == SQLITE_ROW)
		{
			boost::fusion::for_each(row, aux::assign_columns(stmt_, index));
			handler(ec, row);
		}
		if (result != SQLITE_DONE)
		{
			ec.assign(result, get_error_category());
			handler(ec, row);
		}
	}
private:
	boost::reference_wrapper<boost::asio::io_service> io_service_;
	boost::shared_ptr<struct sqlite3> conn_;
	boost::shared_ptr<struct sqlite3_stmt> stmt_;
	boost::system::error_code ec_;
	mutable std::string last_error_;
};

}

}

#endif