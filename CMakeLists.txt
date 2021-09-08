
cmake_minimum_required(VERSION 3.1.3)
project(intel8080-emulator)
set(EXE_NAME i8080emu)

if (MSVC)
	set(ADDITIONAL_FLAGS CACHE INTERNAL "/WX")
else()
	# Should work on most compilers
	set(ADDITIONAL_FLAGS CACHE INTERNAL "-Wall -Wextra -pedantic -Werror")
endif()
add_compile_options(${ADDITIONAL_FLAGS})

option(BUILD_FRONTEND "Build command line frontend (C++11)." ON) # set this off by default eventually
	
if (BUILD_FRONTEND)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_subdirectory(libi8080)

if (BUILD_FRONTEND)
	add_executable(${EXE_NAME} 
		#frontend/test.cpp
		#frontend/util.cpp
		#frontend/cpm80vm.cpp
		#frontend/i8080_exts.cpp
		frontend/main.cpp
		frontend/run_tests.cpp)
	target_include_directories(${EXE_NAME} PRIVATE frontend/include)

	target_link_libraries(${EXE_NAME} i8080)
	file(COPY libi8080/bin DESTINATION ${CMAKE_BINARY_DIR})
endif()
