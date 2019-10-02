cmake_minimum_required(VERSION 3.1.0)
project(i8080emu)
set(EXE_NAME i8080emu)

option(FORCE_INSTALL "Force installation of dependencies if not available." OFF)

# Search for dependencies and auto-install them if possible
if (FORCE_INSTALL)
    if (WIN32)
        message(STATUS "Installing dependencies Chocolatey and git-lfs if not available...")
        execute_process(COMMAND PowerShell -command "Get-PackageProvider -Name Chocolatey -ForceBootstrap; choco install git-lfs;")
    elseif (UNIX)
        message(STATUS "Installing dependency git-lfs if not available...")
        execute_process(COMMAND sudo apt-get --yes install git-lfs)
    else()
        message(FATAL_ERROR "Unrecognized build environment. Install dependencies yourself before running cmake.")
    endif()
endif(FORCE_INSTALL)

# Get test files from git lfs
set(GIT_LFS_INSTALL_AND_PULL cd ${CMAKE_SOURCE_DIR} && git lfs install && git lfs pull)
if (WIN32)
    execute_process(COMMAND ${GIT_LFS_INSTALL_AND_PULL} RESULT_VARIABLE GIT_LFS_SUCCESS)
else()
    execute_process(COMMAND ${GIT_LFS_INSTALL_AND_PULL} RESULT_VARIABLE GIT_LFS_SUCCESS)
endif()
if (NOT GIT_LFS_SUCCESS)
    message(FATAL_ERROR "Unable to pull test files. Download the files manually to libi8080emu/tests. Abort.")
endif()
message(STATUS "Test files pulled successfully.")

# Find system threads library
find_package(Threads REQUIRED)

# Set compiler flags for debug and release mode
# ISO C90 is identical to ANSI C89
set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 11)

if (MSVC)
    set(i8080_DEBUG_FLAGS /W4 /WX /DEBUG:FASTLINK /Od /EHa)
    set(i8080_RELEASE_FLAGS /W4 /WX /O2 /EHsc)
else()
    set(i8080_DEBUG_FLAGS -g -O0 -Wall -Wextra -Werror)
    set(i8080_RELEASE_FLAGS -O3 -Wall -Wextra -Werror -fno-exceptions)
endif()

# make static libaries of libi8080 and libi8080emu
include_directories(libi8080emu/libi8080/include)
add_library(i8080 STATIC libi8080emu/libi8080/src/i8080.c libi8080emu/libi8080/src/i8080_sync.c)
include_directories(libi8080emu/include)
add_library(i8080-wrapper STATIC libi8080emu/src/emu.c libi8080emu/src/emu_debug.c)
# link libi8080emu to libi8080
target_link_libraries(i8080-wrapper PUBLIC i8080)

# Generate build system
add_executable(${EXE_NAME} main.cpp run_tests.cpp)
target_link_libraries(${EXE_NAME} i8080-wrapper)
target_link_libraries(${EXE_NAME} Threads::Threads)
# append to the debug and release flags
target_compile_options(${EXE_NAME} PUBLIC "$<$<CONFIG:DEBUG>:${i8080_DEBUG_FLAGS}>")
target_compile_options(${EXE_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${i8080_RELEASE_FLAGS}>")

# copy test files to the build folder
file(COPY libi8080emu/tests DESTINATION ${CMAKE_BINARY_DIR})