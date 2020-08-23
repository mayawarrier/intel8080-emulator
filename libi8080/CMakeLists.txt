
cmake_minimum_required(VERSION 3.1.3)

# Target is ANSI C or higher, depends on: none
add_library(i8080 src/i8080.c)
target_include_directories(i8080
	PUBLIC
		$<INSTALL_INTERFACE:include/i8080>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/i8080>
	PRIVATE include/
)
