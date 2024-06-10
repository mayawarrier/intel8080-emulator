
cmake_minimum_required(VERSION 3.1)
project(i8080emu)

option(LIBI8080_TEST "Enable tests." OFF)

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to Release.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Select build type." FORCE)
endif()

include(GNUInstallDirs)

add_library(i8080 src/i8080.c)
target_include_directories(i8080 PUBLIC
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_definitions(i8080 PRIVATE _CRT_SECURE_NO_WARNINGS)

if (MSVC)
	target_compile_options(i8080 PRIVATE /W3 /WX)
else()
	target_compile_options(i8080 PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if (LIBI8080_TEST)
	add_subdirectory(tests)
endif()