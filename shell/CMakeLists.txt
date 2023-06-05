cmake_minimum_required(VERSION 3.1)

add_executable(i8080emu src/emu.cpp src/keyintr.cpp src/main.cpp)
	
if (${CMAKE_VERSION} VERSION_GREATER "3.8.0" OR ${CMAKE_VERSION} VERSION_EQUAL "3.8.0")
	target_compile_features(i8080emu PRIVATE cxx_std_11)
endif()

target_include_directories(i8080emu PRIVATE libs/cxxopts/include include)
target_link_libraries(i8080emu i8080)

if (MSVC)
	target_compile_options(i8080emu PRIVATE /W3 /WX)
	target_compile_definitions(i8080emu PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
	target_compile_options(i8080emu PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(MSVC_VERSION GREATER 1910)
	target_compile_options(i8080emu PRIVATE /permissive-)
endif()