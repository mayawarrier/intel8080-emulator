cmake_minimum_required(VERSION 3.1.0)
project(i8080emu)
set(EXE_NAME i8080-emu)

option(LIBS_ONLY "Compile the libi8080emu and libi8080 libraries only." OFF)

# Find system threads library
find_package(Threads REQUIRED)

# Set compiler flags for debug and release mode
# ISO C90 is identical to ANSI C89
set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 11)

if (MSVC)
    set(i8080_DEBUG_FLAGS /W4 /WX /DEBUG /Od /EHa)
    set(i8080_RELEASE_FLAGS /W4 /WX /O2 /EHsc)
else()
    set(i8080_DEBUG_FLAGS -g -O0 -Wall -Wextra -Werror)
    set(i8080_RELEASE_FLAGS -O3 -Wall -Wextra -Werror -fno-exceptions)
endif()

# make static libaries of libi8080 and libi8080emu
include_directories(libi8080emu/libi8080/include)
add_library(i8080 STATIC libi8080emu/libi8080/src/i8080.c libi8080emu/libi8080/src/i8080_sync.c)
include_directories(libi8080emu/include)
add_library(i8080emu STATIC libi8080emu/src/emu.c libi8080emu/src/emu_debug.c)
# compile both libraries
target_compile_options(i8080 PUBLIC "$<$<CONFIG:DEBUG>:${i8080_DEBUG_FLAGS}>"
                                    "$<$<CONFIG:RELEASE>:${i8080_RELEASE_FLAGS}>")
target_compile_options(i8080emu PUBLIC "$<$<CONFIG:DEBUG>:${i8080_DEBUG_FLAGS}>"
                                       "$<$<CONFIG:RELEASE>:${i8080_RELEASE_FLAGS}>")
# link libi8080emu to libi8080
target_link_libraries(i8080emu PUBLIC i8080)

# copy test files to the build folder
file(COPY libi8080emu/tests DESTINATION ${CMAKE_BINARY_DIR})

if (NOT LIBS_ONLY)
    # Generate build system
    add_executable(${EXE_NAME} main.cpp run_tests.cpp)
    target_link_libraries(${EXE_NAME} i8080emu)
    target_link_libraries(${EXE_NAME} Threads::Threads)
    target_compile_options(${EXE_NAME} PUBLIC "$<$<CONFIG:DEBUG>:${i8080_DEBUG_FLAGS}>"
                                              "$<$<CONFIG:RELEASE>:${i8080_RELEASE_FLAGS}>")
endif()
