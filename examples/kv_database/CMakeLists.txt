cmake_minimum_required (VERSION 2.6)
project (kv_database)

find_package (Sqlite REQUIRED)
find_package (Boost REQUIRED COMPONENTS
	system
	thread)

include_directories (
	${Boost_INCLUDE_DIRS})
add_executable (kv_database
	main)
target_link_libraries (kv_database
	${Boost_LIBRARIES}
	${SQLITE_LIBRARIES})
