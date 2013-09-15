#if !defined(SQLITE_SERVICE_AUX_BIND_PARAMS_HPP_)
#define SQLITE_SERVICE_AUX_BIND_PARAMS_HPP_

namespace services { namespace sqlite { namespace aux {

struct bind_params
{
	boost::shared_ptr<struct sqlite3_stmt> & stmt_;
	int & index_;
	bind_params(boost::shared_ptr<struct sqlite3_stmt> & stmt, int & index)
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
	operator()(const T & t) const
	{
		int result = sqlite3_bind_int(stmt_.get(), index_++, t);
		assert(result == SQLITE_OK && "SQLite misuse");
	}
	template <typename T>
	typename boost::enable_if<
		boost::type_traits::ice_and<
			boost::is_arithmetic<T>::value,
			(sizeof(T) == sizeof(sqlite3_int64))
		>,
		void
	>::type
	operator()(const T & t) const
	{
		int result = sqlite3_bind_int64(stmt_.get(), index_++, t);
		assert(result == SQLITE_OK && "SQLite misuse");
	}
	void operator()(const char * t) const
	{
		int result = sqlite3_bind_text(stmt_.get(), index_++, t, -1, SQLITE_TRANSIENT);
		assert(result == SQLITE_OK && "SQLite misuse");
	}
	void operator()(const std::string & t) const
	{
		assert(false);
		int result = sqlite3_bind_text(stmt_.get(), index_++, t.c_str(), t.size(), SQLITE_TRANSIENT);
		assert(result == SQLITE_OK && "SQLite misuse");
	}
};

}
}
}

#endif