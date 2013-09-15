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
	inline typename boost::enable_if<
		boost::type_traits::ice_and<
			boost::is_arithmetic<T>::value,
			(sizeof(T) != sizeof(sqlite3_int64))
		>,
		void
	>::type
	bind(int index, const T & t) const
	{
		int result = sqlite3_bind_int(stmt_.get(), index, t);
		assert(result == SQLITE_OK && "SQLite misuse");
	}
	template <typename T>
	inline typename boost::enable_if<
		boost::type_traits::ice_and<
			boost::is_arithmetic<T>::value,
			(sizeof(T) == sizeof(sqlite3_int64))
		>,
		void
	>::type
	bind(int index, const T & t) const
	{
		int result = sqlite3_bind_int64(stmt_.get(), index, t);
		assert(result == SQLITE_OK && "SQLite misuse");
	}
	inline void bind(int index, const char * t) const
	{
		int result = sqlite3_bind_text(stmt_.get(), index, t, -1, SQLITE_TRANSIENT);
		assert(result == SQLITE_OK && "SQLite misuse");
	}
	inline void bind(int index, const std::string & t) const
	{
		int result = sqlite3_bind_text(stmt_.get(), index, t.c_str(), t.size(), SQLITE_TRANSIENT);
		assert(result == SQLITE_OK && "SQLite misuse");
	}
	inline int lookup(const char * key) const
	{
		int result = ::sqlite3_bind_parameter_index(stmt_.get(), key);
		assert(result > 0 && "Unknown parameter name");
		return result;
	}
	template <typename T>
	void operator()(const std::pair<const char *, T> & args) const
	{
		int idx = lookup(args.first);
		bind(idx, args.second);
	}
	template <typename T>
	void operator()(const std::pair<std::string, T> & args) const
	{
		int idx = lookup(args.first.c_str());
		bind(idx, args.second);
	}
	template <typename T>
	inline void operator()(const T & t) const
	{
		bind(index_++, t);
	}

};

}
}
}

#endif