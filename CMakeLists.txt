
cmake_minimum_required(VERSION 3.1)
project(i8080emu LANGUAGES C CXX)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Select build type." FORCE)
endif()

add_subdirectory(libi8080)


add_executable(i8080emu src/emu.cpp src/keyintr.cpp src/main.cpp)

if (${CMAKE_VERSION} VERSION_GREATER "3.8.0" OR ${CMAKE_VERSION} VERSION_EQUAL "3.8.0")
	target_compile_features(i8080emu PRIVATE cxx_std_11)
endif()

target_include_directories(i8080emu PRIVATE libs/cxxopts/include include)
target_link_libraries(i8080emu i8080)

# copy tests to build directory
add_custom_command(
	TARGET i8080emu
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/tests
		${CMAKE_BINARY_DIR}/tests
)

if (MSVC)
	target_compile_options(i8080emu PRIVATE /W3 /WX)
	target_compile_definitions(i8080emu PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
	target_compile_options(i8080emu PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(MSVC_VERSION GREATER 1910)
	# set strict standard conformance
	target_compile_options(i8080emu PRIVATE /permissive-)
endif()