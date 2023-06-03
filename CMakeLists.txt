
cmake_minimum_required(VERSION 3.1)
project(i8080emu LANGUAGES C CXX)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type provided, default to Release.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Select build type." FORCE)
endif()

add_subdirectory(libi8080)
add_subdirectory(shell)
