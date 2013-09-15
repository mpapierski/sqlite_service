#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <boost/asio.hpp>
#include "sqlite_service/sqlite_service.hpp"

/**
 * Mock handlers
 */

struct Client
{
	MOCK_METHOD1(handle_open, void(const boost::system::error_code & /* ec */));
	MOCK_METHOD1(handle_exec, void(const boost::system::error_code & /* ec */));
	MOCK_METHOD1(handle_prepare, void(services::sqlite::statement));
};

struct ServiceTest : ::testing::Test
{
	ServiceTest()
		: timeout_timer(io_service)
		, database(io_service)
	{
	}
	void SetUp()
	{
		timeout_timer.expires_from_now(boost::posix_time::seconds(5));
		timeout_timer.async_wait(boost::bind(&boost::asio::io_service::stop, &io_service));
	}
	void TearDown()
	{
		timeout_timer.cancel();
	}
	Client client;
	boost::asio::io_service io_service;
	boost::asio::deadline_timer timeout_timer;
	services::sqlite::database database;
};

struct ServiceTestMemory : ServiceTest
{
	ServiceTestMemory()
	{
		database.open(":memory:");
	}
};

using ::testing::SaveArg;
using ::testing::SaveArg;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::_;
using ::testing::StrEq;
using ::testing::Return;
using ::testing::NotNull;

ACTION_P(SetPointer1, n) { return *arg1 = n; }

TEST_F (ServiceTest, AsyncOpen)
{
	boost::system::error_code ec;
	EXPECT_CALL(client, handle_open(_))
		.WillOnce(DoAll(
			SaveArg<0>(&ec),
			Invoke(boost::bind(&boost::asio::io_service::stop, &io_service))));
	database.async_open(":memory:", boost::bind(&Client::handle_open, &client, boost::asio::placeholders::error()));
	io_service.run();
	ASSERT_FALSE(ec);
}

TEST_F (ServiceTest, UnableToExecuteQuery)
{
	boost::system::error_code ec;
	EXPECT_CALL(client, handle_exec(_))
		.WillOnce(DoAll(
			SaveArg<0>(&ec),
			Invoke(boost::bind(&boost::asio::io_service::stop, &io_service))));
	database.async_exec("CREATE TABLE asdf (value1, value2, value3)", boost::bind(&Client::handle_exec, &client, boost::asio::placeholders::error()));
	io_service.run();
	ASSERT_TRUE(ec);
}

TEST_F (ServiceTest, UnableToFetchQuery)
{
	boost::system::error_code ec;
	EXPECT_CALL(client, handle_exec(_))
		.WillOnce(DoAll(
			SaveArg<0>(&ec),
			Invoke(boost::bind(&boost::asio::io_service::stop, &io_service))));
	database.async_fetch("SELECT 1", boost::bind(&Client::handle_exec, &client, boost::asio::placeholders::error()));
	io_service.run();
	ASSERT_TRUE(ec);
}

TEST_F (ServiceTestMemory, ExecuteInvalidQuery)
{
	boost::system::error_code ec;
	EXPECT_CALL(client, handle_exec(_))
		.WillOnce(DoAll(
			SaveArg<0>(&ec),
			Invoke(boost::bind(&boost::asio::io_service::stop, &io_service))));
	database.async_exec("this is invalid query", boost::bind(&Client::handle_exec, &client, boost::asio::placeholders::error()));
	io_service.run();
	ASSERT_TRUE(ec);
	EXPECT_EQ("SQL logic error or missing database", ec.message());
}

TEST_F (ServiceTestMemory, ExecuteSimpleQuery)
{
	boost::system::error_code ec;
	EXPECT_CALL(client, handle_exec(_))
		.WillOnce(DoAll(
			SaveArg<0>(&ec),
			Invoke(boost::bind(&boost::asio::io_service::stop, &io_service))));
	database.async_fetch("SELECT 1", boost::bind(&Client::handle_exec, &client, boost::asio::placeholders::error()));
	io_service.run();
	ASSERT_FALSE(ec) << ec.message();
}

TEST_F (ServiceTestMemory, FetchMultipleRows)
{
	::testing::InSequence seq;
	boost::system::error_code ec1, ec2, ec3;
	EXPECT_CALL(client, handle_exec(_))
		.WillOnce(SaveArg<0>(&ec1));
	EXPECT_CALL(client, handle_exec(_))
		.WillOnce(SaveArg<0>(&ec2));
	EXPECT_CALL(client, handle_exec(_))
		.WillOnce(DoAll(
			SaveArg<0>(&ec3),
			Invoke(boost::bind(&boost::asio::io_service::stop, &io_service))));
	database.async_fetch("SELECT 1 UNION SELECT 2 UNION SELECT 3", boost::bind(&Client::handle_exec, &client, boost::asio::placeholders::error()));
	io_service.run();
	ASSERT_FALSE(ec1);
	ASSERT_FALSE(ec2);
	ASSERT_FALSE(ec3);
}

TEST_F (ServiceTestMemory, PrepareTest)
{
	boost::system::error_code ec;
	typedef boost::tuple<int, long, boost::uint64_t, std::string, std::string> result_t;
	services::sqlite::statement stmt = database.prepare("SELECT 1, 2, 3, 'hello world', NULL");
	bool ok;
	result_t row;
	ok = stmt.fetch(row);
	ASSERT_TRUE(ok);
	EXPECT_EQ(1, boost::get<0>(row));
	EXPECT_EQ(2, boost::get<1>(row));
	EXPECT_EQ(3, boost::get<2>(row));
	EXPECT_EQ("hello world", boost::get<3>(row));
	EXPECT_EQ(std::string(), boost::get<4>(row));
	ok = stmt.fetch(row);
	ASSERT_FALSE(ok);
}

TEST_F (ServiceTestMemory, PrepareTestMultiRow)
{
	boost::system::error_code ec;
	typedef boost::tuple<std::string> result_t;
	services::sqlite::statement stmt = database.prepare(
		"SELECT 'hello' UNION "
		"SELECT 'world'");
	bool ok;
	result_t row;
	ok = stmt.fetch(row);
	ASSERT_TRUE(ok);
	EXPECT_EQ("hello", boost::get<0>(row));
	boost::get<0>(row).clear();
	ok = stmt.fetch(row);
	EXPECT_EQ("world", boost::get<0>(row));
	boost::get<0>(row).clear();
	ok = stmt.fetch(row);
	ASSERT_FALSE(ok);
	EXPECT_EQ(std::string(), boost::get<0>(row));
}

TEST_F (ServiceTestMemory, AsyncPrepareStatement)
{
	typedef boost::tuple<int, std::string> row_t;
	services::sqlite::statement stmt;
	EXPECT_CALL(client, handle_prepare(_))
		.WillOnce(DoAll(SaveArg<0>(&stmt), Invoke(boost::bind(&boost::asio::io_service::stop, &io_service))));
	database.async_prepare("SELECT 1", boost::bind(&Client::handle_prepare, &client, _1));
	io_service.run();
	ASSERT_FALSE(stmt.error());
	EXPECT_EQ(std::string(), stmt.last_error());
}

TEST_F (ServiceTestMemory, BlockingPrepareStatementWithBind)
{
	typedef boost::tuple<int, std::string> row_t;
	services::sqlite::statement stmt;
	stmt = database.prepare("SELECT ? + 1, 'hello ' || ?");
	stmt.bind_params(boost::make_tuple(41, "world"));
	ASSERT_FALSE(stmt.error());
	EXPECT_EQ(std::string(), stmt.last_error());
	row_t row;
	bool ok = stmt.fetch(row);
	EXPECT_TRUE(ok);
	EXPECT_EQ(42, boost::get<0>(row));
	EXPECT_EQ("hello world", boost::get<1>(row));
}


TEST_F (ServiceTestMemory, AsyncPrepareStatementFailure)
{
	typedef boost::tuple<int, std::string> row_t;
	services::sqlite::statement stmt;
	EXPECT_CALL(client, handle_prepare(_))
		.WillOnce(DoAll(SaveArg<0>(&stmt), Invoke(boost::bind(&boost::asio::io_service::stop, &io_service))));
	database.async_prepare("I dont know what I am doing", boost::bind(&Client::handle_prepare, &client, _1));
	io_service.run();
	ASSERT_TRUE(stmt.error());
	EXPECT_EQ(std::string("near \"I\": syntax error"), stmt.last_error());
}