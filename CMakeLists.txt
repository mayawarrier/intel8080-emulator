cmake_minimum_required(VERSION 3.1.0)
project(i8080emu)

set(CMAKE_C_STANDARD, 89)
set(CMAKE_CXX_STANDARD, 11)
set(EXE_NAME i8080emu)

# make static libaries of libi8080 and libi8080emu
include_directories(libi8080emu/libi8080/include)
add_library(libi8080 STATIC libi8080emu/libi8080/src/i8080.c libi8080emu/libi8080/src/i8080_sync.c)
include_directories(libi8080emu/include)
add_library(libi8080emu STATIC libi8080emu/src/emu.c libi8080emu/src/emu_debug.c)
# link libi8080emu to libi8080
target_link_libraries(libi8080emu PUBLIC libi8080)

# Find and link system threads libary
find_package(Threads REQUIRED)

add_executable(${EXE_NAME} main.cpp run_tests.cpp)
target_link_libraries(${EXE_NAME} libi8080emu)
target_link_libraries(${EXE_NAME} Threads::Threads)