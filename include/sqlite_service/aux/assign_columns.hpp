#if !defined(SQLITE_SERVICE_AUX_ASSIGN_COLUMNS_HPP_)
#define SQLITE_SERVICE_AUX_ASSIGN_COLUMNS_HPP_

namespace services { namespace sqlite { namespace aux {

class assign_columns
{
public:
	assign_columns(const boost::shared_ptr<struct sqlite3_stmt> & stmt, int & index)
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
		t = sqlite3_column_int(stmt_.get(), index_++);
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
		t = sqlite3_column_int(stmt_.get(), index_++);
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
		const unsigned char * text = sqlite3_column_text(stmt_.get(), index_++);
		if (!text)
		{
			t = T();
		}
		else
		{
			t = reinterpret_cast<const char *>(text);
		}
	}
private:
	const boost::shared_ptr<struct sqlite3_stmt> & stmt_;
	int & index_;
};
}

}

}

#endif