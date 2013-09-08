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
	services::sqlite::database database(io_service);
	database.async_open(":memory:", boost::bind(&Client::handle_open, &client, boost::asio::placeholders::error()));
	io_service.run();
	ASSERT_FALSE(ec);
}

TEST_F (ServiceTest, ExecuteInvalidQuery)
{
	boost::system::error_code ec;
	EXPECT_CALL(client, handle_exec(_))
		.WillOnce(DoAll(
			SaveArg<0>(&ec),
			Invoke(boost::bind(&boost::asio::io_service::stop, &io_service))));
	services::sqlite::database database(io_service);
	database.async_exec("this is invalid query", boost::bind(&Client::handle_exec, &client, boost::asio::placeholders::error()));
	io_service.run();
	ASSERT_TRUE(ec);
	EXPECT_EQ("library routine called out of sequence", ec.message());
}
