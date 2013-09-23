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
	static pointer create(boost::asio::io_service & io_service,
		services::sqlite::database & db)
	{
		return pointer(new connection(io_service, db));
	}
	inline tcp::socket & socket()
	{
		return socket_;
	}
	void start_read()
	{
		std::cout << "start read" << std::endl;
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
		}
		else
		{
			std::cerr << error.message() << std::endl;
		}
	}
	void handle_line(const std::string & input)
	{
		std::cout << "Received: " << input << std::endl;
		std::istringstream iss(input);
		std::string method, key, value;
		iss >> method >> key;
		if (method == "GET" && !key.empty())
		{
			db_.async_prepare("SELECT value FROM storage WHERE key = ?",
				boost::bind(&connection::handle_prepare, shared_from_this(), key, _1));
		}
	}
	void handle_prepare(::std::string key,
		services::sqlite::statement stmt)
	{
		if (stmt.error())
		{
			std::cerr << "NOT PREPARED" << std::endl;
			return;	
		}
		stmt.bind_params(boost::make_tuple(key));
		stmt.async_fetch<boost::tuple<std::string> >(
			boost::bind(&connection::handle_get_result, this,
				boost::asio::placeholders::error(),
				_2));
	}
	void handle_get_result(const boost::system::error_code & err, boost::tuple<std::string> row)
	{
		if (err)
		{
			std::cerr << "Unable to retrieve result set" << std::endl;
			return;
		}
		boost::asio::async_write(socket_, boost::asio::buffer(boost::get<0>(row) + "\r\n"),
			boost::bind(&connection::handle_write, shared_from_this(),
				boost::asio::placeholders::error(),
				boost::asio::placeholders::bytes_transferred()));
	}
	void handle_write(const boost::system::error_code& error,
		std::size_t bytes_transferred)
	{
	} 
private:
	connection(boost::asio::io_service & io_service,
			services::sqlite::database & db)
		: io_service_(io_service)
		, socket_(io_service_)
		, db_(db)
	{

	}
	boost::asio::io_service & io_service_;
	tcp::socket socket_;
	boost::asio::streambuf buffer_;
	services::sqlite::database & db_;
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
		connection::pointer new_connection = connection::create(io_service_, db_);
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
		std::cout << "New connection." << std::endl;
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
