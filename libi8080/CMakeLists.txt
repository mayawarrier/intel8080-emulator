
cmake_minimum_required(VERSION 3.1)

include(GNUInstallDirs)

add_library(i8080 src/i8080.c)
target_include_directories(i8080 PUBLIC
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if (MSVC)
	target_compile_options(i8080 PRIVATE /W3 /WX)
else()
	target_compile_options(i8080 PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
