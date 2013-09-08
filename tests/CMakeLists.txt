# Unit tests
find_package (Boost REQUIRED COMPONENTS
	system
	thread)
add_subdirectory (gmock)
include_directories (
	${Boost_INCLUDE_DIRS}
	gmock/include/
	gtest/include/
)
add_executable (tests tests.cpp)
target_link_libraries (tests
	${Boost_LIBRARIES}
	gmock
	gtest
	gtest_main
	sqlite3)
add_test (tests tests)
