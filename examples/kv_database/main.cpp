#include <boost/asio.hpp>
#include <string>
#include <boost/array.hpp>
#include "sqlite_service/sqlite_service.hpp"

using boost::asio::ip::tcp;

class connection
	: public boost::enable_shared_from_this<connection>
{
public:
	typedef boost::shared_ptr<connection> pointer;
	static pointer create(boost::asio::io_service & io_service)
	{
		return pointer(new connection(io_service));
	}
	inline tcp::socket & socket()
	{
		return socket_;
	}
	void start_read()
	{
		boost::asio::async_read_until(
			socket_,
			buffer_,
			"\r\n",
			boost::bind(&connection::handle_read,
				shared_from_this(),
				boost::asio::placeholders::error(),
				boost::asio::placeholders::bytes_transferred()));
	}
	void handle_read(const boost::system::error_code & error, std::size_t bytes_transferred)
	{
		if (!error && bytes_transferred > 0)
		{
			std::istream is(&buffer_);
			std::string line;
			if (std::getline(is, line))
			{
				handle_line(line);
			}
			start_read();
			return;
		}
	}
	void handle_line(const std::string & input)
	{
		std::cout << "Received: " << input << std::endl;
		std::istringstream iss(input);
		std::string method, key, value;
		iss >> method >> key >> value;
		if (method == "GET" && !key.empty() && !value.empty())
		{

		}
	}
private:
	connection(boost::asio::io_service & io_service)
		: io_service_(io_service)
		, socket_(io_service_)
	{

	}
	boost::asio::io_service & io_service_;
	tcp::socket socket_;
	boost::asio::streambuf buffer_;
};

class storage
{
public:
	storage(boost::asio::io_service & io_service)
		: io_service_(io_service)
		, db_(io_service)
		, acceptor_(io_service_, tcp::endpoint(tcp::v4(), 9999))
	{
	}
	void start(const ::std::string & url)
	{
		db_.async_open(url, boost::bind(&storage::handle_open, this,
			boost::asio::placeholders::error()));
	}
	void start_accept()
	{
		connection::pointer new_connection = connection::create(io_service_);
		acceptor_.async_accept(new_connection->socket(),
			boost::bind(&storage::handle_accept, this,
			new_connection,
			boost::asio::placeholders::error()));
	}
	/**
	 * Database connection is opened.
	 */
	void handle_open(boost::system::error_code & ec)
	{
		if (ec)
		{
			std::cerr << "Failed to open database connection: " << ec.message () << '.' << std::endl;
			return;
		}
		std::cout << "Database connection opened!" << std::endl;
			db_.async_exec("CREATE TABLE IF NOT EXISTS storage (key TEXT PRIMARY KEY NOT NULL, value)",
				boost::bind(&storage::handle_exec_result, this,
					boost::asio::placeholders::error()));
	}
	/**
	 * Database created.
	 */
	void handle_exec_result(const boost::system::error_code & err)
	{
		if (err)
		{
			std::cerr << "Unable to create database: " << err.message() << '.' << std::endl;
			return;
		}
		std::cout << "Database created." << std::endl;
		start_accept();
	}
	/**
	 * New connection
	 */
	void handle_accept(connection::pointer new_connection,
		const boost::system::error_code & ec)
	{
		if (ec)
		{
			std::cerr << "Unable to accept new connection: " << ec.message() << '.' << std::endl;
			return;
		}
		new_connection->start_read();
		start_accept();
	}
private:
	boost::asio::io_service & io_service_;
	/** Database service */
	services::sqlite::database db_;
	/** Connection acceptor */
	boost::asio::ip::tcp::acceptor acceptor_;
};

int
main(int argc, char * argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " [database]" << std::endl;
		return 1;
	}
	// Setup main io service
	boost::asio::io_service io_service;
	// KV store
	storage kv(io_service);
	// Start asynchronous KV store
	kv.start(argv[1]);
	// Run service
	io_service.run();
}
