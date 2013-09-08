#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <boost/asio.hpp>
#include "sqlite_service/sqlite_service.hpp"

/**
	* Mock handlers */
struct Client
{
	MOCK_METHOD1(handle_open, void(const boost::system::error_code & /* ec */));
};

struct ServiceTest : ::testing::Test
{
	Client client;
};

using ::testing::SaveArg;
using ::testing::_;

TEST_F (ServiceTest, AsyncOpen)
{
	boost::system::error_code ec;
	EXPECT_CALL(client, handle_open(_))
		.WillOnce(SaveArg<0>(&ec));
	boost::asio::io_service io_service;
	services::sqlite::database database(io_service);
	database.async_open(":memory:", boost::bind(&Client::handle_open, &client, boost::asio::placeholders::error()));
	io_service.run();
	ASSERT_FALSE(ec);
}
