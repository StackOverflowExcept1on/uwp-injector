cmake_minimum_required(VERSION 3.11)
project(uwp_injector)

# Checking for MSVC toolchain with C++20 features

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
    message(FATAL_ERROR "MSVC toolchain is required!")
endif ()

# Loading all kinds of projects
add_subdirectory(injector)
add_subdirectory(library)
