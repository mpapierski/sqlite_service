# Unit tests

find_package (Boost REQUIRED COMPONENTS
	system
	thread)

include_directories (${Boost_INCLUDE_DIRS})
add_executable (tests tests.cpp)
target_link_libraries (tests ${Boost_LIBRARIES} sqlite3)
