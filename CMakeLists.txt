cmake_minimum_required(VERSION 3.1.0)
project(i8080emulator)
set(EXE_NAME i8080emu)

option(DISABLE_INTERRUPTS "Disable async interrupts emulation if no mutex type is available." OFF)

if(NOT DISABLE_INTERRUPTS)
    # Find system threads library
    find_package(Threads REQUIRED)
endif()

if (MSVC)
    set(i8080_DEBUG_FLAGS /W4 /WX /DEBUG /Od /EHa)
    set(i8080_RELEASE_FLAGS /W4 /WX /O2 /EHsc)
else()
    set(i8080_DEBUG_FLAGS -g -O0 -Wall -Wextra -Werror)
    set(i8080_RELEASE_FLAGS -O3 -Wall -Wextra -Werror -fno-exceptions)
endif()

# make static libaries of libi8080 and libcpm80vm
include_directories(libi8080/include)
include_directories(libi8080/include/i8080)
add_library(i8080 STATIC libi8080/src/i8080.c)
target_compile_options(i8080 PUBLIC "$<$<CONFIG:DEBUG>:${i8080_DEBUG_FLAGS}>" "$<$<CONFIG:RELEASE>:${i8080_RELEASE_FLAGS}>")

#include_directories(libcpm80vm/include)
#add_library(cpm80vm STATIC libcpm80vm/src/emu.c libi8080emu/src/emu_debug.c)
#target_compile_options(i8080emu PUBLIC "$<$<CONFIG:DEBUG>:${i8080_DEBUG_FLAGS}>" "$<$<CONFIG:RELEASE>:${i8080_RELEASE_FLAGS}>")
#target_link_libraries(i8080emu PUBLIC i8080)

# copy test files to the build folder
file(COPY libcpm80vm/tests DESTINATION ${CMAKE_BINARY_DIR})

# Generate build system
#add_executable(${EXE_NAME} frontends/shell/main.cpp frontends/shell/run_tests.cpp)
#target_link_libraries(${EXE_NAME} i8080)
#if (NOT DISABLE_INTERRUPTS)
#    target_link_libraries(${EXE_NAME} Threads::Threads)
#endif()
#target_compile_options(${EXE_NAME} PUBLIC "$<$<CONFIG:DEBUG>:${i8080_DEBUG_FLAGS}>" "$<$<CONFIG:RELEASE>:${i8080_RELEASE_FLAGS}>")