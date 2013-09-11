# Unit tests
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)
find_package (Sqlite REQUIRED)
find_package (Boost REQUIRED COMPONENTS
	system
	thread)
add_subdirectory (gmock)
include_directories (
	${Boost_INCLUDE_DIRS}
	${SQLITE_LIBRARIES}
	gmock/include/
	gtest/include/
)
add_definitions (${SQLITE_DEFINITIONS})
add_executable (tests tests.cpp)
target_link_libraries (tests
	${Boost_LIBRARIES}
	${SQLITE_LIBRARIES}
	gmock
	gtest
	gtest_main)
add_test (tests tests)
