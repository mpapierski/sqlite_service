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

#include "sqlite3.h"

namespace services { namespace sqlite {

class service;

/**
 * Statement wrapper
 */
template <typename TupleType>
class statement
{
private:
	BOOST_MOVABLE_BUT_NOT_COPYABLE(statement)

public:
	typedef TupleType tuple_type;
	statement(boost::shared_ptr<struct sqlite3> conn, const ::std::string & query)
		: conn_(conn)
		, stmt_(NULL)
	{
		int result;
		boost::system::error_code ec;
		if ((result = sqlite3_prepare(conn_.get(), query.c_str(), -1, &stmt_, NULL)) != SQLITE_OK)
		{
			ec.assign(result, get_error_category());
		}
		if (ec)
		{
			throw boost::system::system_error(ec, ::sqlite3_errmsg(conn_.get()));
		}
	}
	~statement()
	{
		finalize();
	}
	statement(BOOST_RV_REF(statement) x)            // Move ctor
		: stmt_(x.stmt_)
		, conn_(x.conn_)
	{
		x.stmt_ = 0;
	}
	statement& operator=(BOOST_RV_REF(statement) x) // Move assign
	{
		if (stmt_) finalize();
		stmt_   = x.stmt_;
		x.stmt_ = 0;
		conn_ = x.conn_;
		return *this;
	}
	int step()
	{
		return sqlite3_step(stmt_);
	}
	bool fetch(tuple_type & results)
	{
		int result = step();
		
		if (result == SQLITE_ROW)
		{
			int index = 0;
			boost::fusion::for_each(results, assign_columns(stmt_, index));
		}
		return result == SQLITE_ROW;
	}
private:
	inline void finalize()
	{
		int result = sqlite3_finalize(stmt_);
		assert(result == SQLITE_OK);
	}
	boost::shared_ptr<struct sqlite3> conn_;
	struct sqlite3_stmt * stmt_;
	struct assign_columns
	{
		struct sqlite3_stmt * & stmt_;
		int & index_;
		assign_columns(struct sqlite3_stmt * & stmt, int & index)
			: stmt_(stmt)
			, index_(index)
		{
		}
		template <typename T>
		typename boost::enable_if<
			boost::type_traits::ice_and<
				boost::is_arithmetic<T>::value,
				(sizeof(T) != sizeof(sqlite3_int64))
			>,
			void
		>::type
		operator()(T&t) const
		{
			t = sqlite3_column_int(stmt_, index_++);
		}
		template <typename T>
		typename boost::enable_if<
			boost::type_traits::ice_and<
				boost::is_arithmetic<T>::value,
				(sizeof(T) == sizeof(sqlite3_int64))
			>,
			void
		>::type
		operator()(T&t) const
		{
			t = sqlite3_column_int(stmt_, index_++);
		}
		template <typename T>
		typename boost::enable_if<
			boost::type_traits::ice_and<
				boost::type_traits::ice_not<
					boost::is_arithmetic<T>::value
				>::value,
				boost::is_convertible<const char *, T>::value
			>,
			void
		>::type
		operator()(T&t) const
		{
			const unsigned char * text = sqlite3_column_text(stmt_, index_++);
			if (!text)
			{
				t = T();
			}
			else
			{
				t = reinterpret_cast<const char *>(text);
			}
		}
	};
};

}

}

#endif