#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <boost/asio.hpp>
#include "sqlite_service/sqlite_service.hpp"

/**
	* Mock handlers */
struct Client
{
	MOCK_METHOD1(handle_open, void(const boost::system::error_code & /* ec */));
	MOCK_METHOD1(handle_exec, void(const boost::system::error_code & /* ec */));
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
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::_;

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
