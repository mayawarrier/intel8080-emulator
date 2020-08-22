
cmake_minimum_required(VERSION 3.1.0)
project(intel8080-emulator)
set(EXE_NAME i8080cpm)

set(ADDITIONAL_CFLAGS "Wall" "Wextra" "Werror")

if(NOT MSVC)
	include(CheckCCompilerFlag)
	foreach(ADD_CFLAG IN LISTS ADDITIONAL_CFLAGS)
		set(HYPHEN_ADD_CFLAG "-${ADD_CFLAG}")
		check_c_compiler_flag(${HYPHEN_ADD_CFLAG} SUPPORTS_${ADD_CFLAG})
		if(SUPPORTS_${ADD_CFLAG})
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${HYPHEN_ADD_CFLAG}")
		endif()
	endforeach()
endif()

option(BUILD_FRONTEND "Build command line frontend. Needs C++11 compiler or above." ON)

if (BUILD_FRONTEND)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_subdirectory(libi8080)
add_subdirectory(libcpm80vm)

if (BUILD_FRONTEND)
	add_executable(${EXE_NAME} frontend/test_i8080.cpp 
							   frontend/test_cpm80vm.cpp
							   frontend/test_utils.cpp
							   frontend/main.cpp)
	target_include_directories(${EXE_NAME} PRIVATE frontend)
	#target_include_directories(${EXE_NAME} PRIVATE libcpm80vm/examples)

	target_link_libraries(${EXE_NAME} cpm80vm)
	target_link_libraries(${EXE_NAME} i8080)
	file(COPY libi8080/bin DESTINATION ${CMAKE_BINARY_DIR})
endif()
