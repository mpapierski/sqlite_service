#include <boost/asio.hpp>
#include "sqlite_service/sqlite_service.hpp"

void handle_database_open(const boost::system::error_code & ec)
{
	if (ec)
	{
		std::cout << "ERROR: " << ec.message() << std::endl;
	}
	else
	{
		std::cout << "OK" << std::endl;
	}
}

int
main()
{
	boost::asio::io_service io_service;
	services::sqlite::database database(io_service);
	database.async_open(":memory:", &handle_database_open);
	io_service.run();
}