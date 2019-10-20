cmake_minimum_required(VERSION 3.1.0)
project(i8080emu)
set(EXE_NAME i8080-emu)

option(FIRST_TIME_SETUP "Force installation of dependencies if not available, and pull test files with git lfs." OFF)
option(LIBS_ONLY "Compile the libi8080emu and libi8080 libraries only." OFF)

# Search for dependencies and auto-install them if possible
if (FIRST_TIME_SETUP)

    set(CHECK_IF_GIT_LFS_EXISTS git-lfs --version)
    execute_process(COMMAND ${CHECK_IF_GIT_LFS_EXISTS}
                        RESULT_VARIABLE DEP_SUCCESS)

    # attempt to install if not available
    if (NOT DEP_SUCCESS)
        set(WIN_CHECK_IF_CHOCO_EXISTS choco --version)
        set(WIN_CHOCO_INSTALL runas powershell.exe -ExecutionPolicy Bypass -NoProfile
                                        -command "iwr https://chocolatey.org/install.ps1 -UseBasicParsing | iex")
        set(WIN_CHOCO_GIT_LFS_INSTALL runas powershell.exe -command "choco install -y git-lfs; refreshenv")
        set(UNIX_GIT_LFS_INSTALL sudo apt-get --yes git-lfs)
        set(GIT_LFS_INIT_AND_PULL cd ${CMAKE_SOURCE_DIR} && git lfs install && git lfs pull)

        if (WIN32)
            message(STATUS "Installing dependencies Chocolatey and git-lfs if not available...")
            execute_process(COMMAND ${WIN_CHECK_IF_CHOCO_EXISTS}
                            RESULT_VARIABLE CHOCO_AVAILABLE)
            if (NOT CHOCO_AVAILABLE)
                # If chocolatey is not available, install it and git-lfs
                execute_process(COMMAND ${WIN_CHOCO_INSTALL}
                                RESULT_VARIABLE DEP_SUCCESS)
                if (DEP_SUCCESS)
                    execute_process(COMMAND ${WIN_CHOCO_GIT_LFS_INSTALL}
                                    RESULT_VARIABLE DEP_SUCCESS)
                endif()
            else()
                # install through choco
                execute_process(COMMAND ${WIN_CHOCO_GIT_LFS_INSTALL}
                                RESULT_VARIABLE DEP_SUCCESS)
            endif()

        elseif (UNIX)
            message(STATUS "Installing dependency git-lfs if not available...")
                execute_process(COMMAND ${UNIX_GIT_LFS_INSTALL}
                                RESULT_VARIABLE DEP_SUCCESS)
        else()
            message(FATAL_ERROR "Unrecognized build environment. Install dependencies yourself before running cmake.")
        endif()
    endif()

    # Get test files from git lfs
    if (DEP_SUCCESS)
        message(STATUS "Initializing git-lfs and pulling test files...")
        execute_process(COMMAND ${GIT_LFS_INIT_AND_PULL}
                        RESULT_VARIABLE GIT_LFS_SUCCESS)
        if (GIT_LFS_SUCCESS)
            message(STATUS "libi8080emu test files pulled successfully.")
        else()
            message(FATAL_ERROR "Unable to pull test files. Download the files manually to libi8080emu/tests. Abort.")
        endif()
    else()
        message(SEND_ERROR "Installation of dependencies failed.")
    endif()
endif(FIRST_TIME_SETUP)

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
