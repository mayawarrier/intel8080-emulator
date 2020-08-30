
cmake_minimum_required(VERSION 3.1.3)
project(intel8080-emulator)
set(EXE_NAME i8080cpm)

if (MSVC)
	set(ADDITIONAL_FLAGS CACHE INTERNAL "/WX")
else()
	# These work on most compilers
	set(ADDITIONAL_FLAGS CACHE INTERNAL "-Wall -Wextra -pedantic -Werror")
endif()
add_compile_options(${ADDITIONAL_FLAGS})

option(BUILD_FRONTEND "Build command line frontend. Needs C++11 compiler or above." ON) # set this off by default eventually
	
if (BUILD_FRONTEND)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(BUILD_CPM80_SIMPLE_VM ON)
endif()

add_subdirectory(libi8080)
add_subdirectory(libcpm80vm)

if (BUILD_FRONTEND)
	add_executable(${EXE_NAME} frontend/test_i8080.cpp 
							   frontend/test_cpm80vm.cpp
							   frontend/test_utils.cpp
							   frontend/main.cpp)
	target_include_directories(${EXE_NAME} PRIVATE frontend)

	target_link_libraries(${EXE_NAME} cpm80vm i8080) #$<TARGET_OBJECTS:simple_vm>
	file(COPY libi8080/bin DESTINATION ${CMAKE_BINARY_DIR})
endif()
