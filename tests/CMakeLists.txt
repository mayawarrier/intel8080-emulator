
cmake_minimum_required(VERSION 3.1)

add_executable(i8080emu emu.cpp keyintr.cpp main.cpp)
target_include_directories(i8080emu PRIVATE cxxopts/include ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(i8080emu PRIVATE i8080)

if (${CMAKE_VERSION} VERSION_GREATER "3.8.0" OR ${CMAKE_VERSION} VERSION_EQUAL "3.8.0")
	target_compile_features(i8080emu PRIVATE cxx_std_11)
endif()

include(FetchContent)

FetchContent_Declare(cxxopts
    GIT_REPOSITORY  https://github.com/jarro2783/cxxopts
    GIT_TAG         origin/master)
FetchContent_MakeAvailable(cxxopts)

target_link_libraries(i8080emu PRIVATE cxxopts)

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

# copy tests to build directory
add_custom_command(
	TARGET i8080emu
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/bin
		${CMAKE_CURRENT_BINARY_DIR}/testbin
)