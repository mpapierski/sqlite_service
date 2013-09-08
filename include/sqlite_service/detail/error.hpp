#if !defined(SQLITE_SERVICE_DETAIL_ERROR_HPP_)
#define SQLITE_SERVICE_DETAIL_ERROR_HPP_

namespace services { namespace sqlite { namespace detail {

struct error_category
	: boost::system::error_category
{
	const char * name() const
	{
		return "sqlite3";
	}
	std::string message(int rc) const
	{
		// 3.7.16
#if SQLITE_VERSION_NUMBER >= 3007016
		return ::sqlite3_errstr(rc);
#else
		static boost::array<const char *, 27> aMsg = {{
			/* SQLITE_OK          */ "not an error",
			/* SQLITE_ERROR       */ "SQL logic error or missing database",
			/* SQLITE_INTERNAL    */ 0,
			/* SQLITE_PERM        */ "access permission denied",
			/* SQLITE_ABORT       */ "callback requested query abort",
			/* SQLITE_BUSY        */ "database is locked",
			/* SQLITE_LOCKED      */ "database table is locked",
			/* SQLITE_NOMEM       */ "out of memory",
			/* SQLITE_READONLY    */ "attempt to write a readonly database",
			/* SQLITE_INTERRUPT   */ "interrupted",
			/* SQLITE_IOERR       */ "disk I/O error",
			/* SQLITE_CORRUPT     */ "database disk image is malformed",
			/* SQLITE_NOTFOUND    */ "unknown operation",
			/* SQLITE_FULL        */ "database or disk is full",
			/* SQLITE_CANTOPEN    */ "unable to open database file",
			/* SQLITE_PROTOCOL    */ "locking protocol",
			/* SQLITE_EMPTY       */ "table contains no data",
			/* SQLITE_SCHEMA      */ "database schema has changed",
			/* SQLITE_TOOBIG      */ "string or blob too big",
			/* SQLITE_CONSTRAINT  */ "constraint failed",
			/* SQLITE_MISMATCH    */ "datatype mismatch",
			/* SQLITE_MISUSE      */ "library routine called out of sequence",
			/* SQLITE_NOLFS       */ "large file support is disabled",
			/* SQLITE_AUTH        */ "authorization denied",
			/* SQLITE_FORMAT      */ "auxiliary database format error",
			/* SQLITE_RANGE       */ "bind or column index out of range",
			/* SQLITE_NOTADB      */ "file is encrypted or is not a database",
		}};
		const char *zErr = "unknown error";
		switch( rc ){
			default: {
				rc &= 0xff;
				if(rc>=0 && rc<aMsg.size() && aMsg[rc]!=0 ){
					zErr = aMsg[rc];
				}
				break;
			}
		}
		return zErr;
#endif
	}
};

} // end namespace detail

inline boost::system::error_category & get_error_category()
{
	static detail::error_category category;
	return category;
}

} }

#endif