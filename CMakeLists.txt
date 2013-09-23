cmake_minimum_required (VERSION 2.8.7)
project (sqlite_service)

option (BUILD_TESTS "Build test suite" OFF)
option (BUILD_EXAMPLES "Build examples" OFF)

include_directories (
	include/
)

if (BUILD_TESTS)
	enable_testing ()
	add_subdirectory (tests)
endif ()

if (BUILD_EXAMPLES)
	add_subdirectory (examples)
endif ()