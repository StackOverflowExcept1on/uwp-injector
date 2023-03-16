cmake_minimum_required(VERSION 3.11)
project(library)

# Checking for MSVC toolchain with C++20 features & targeting for Windows Store

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
    message(FATAL_ERROR "MSVC toolchain is required!")
endif ()

if (NOT WINDOWS_STORE)
    message(FATAL_ERROR "It is supposed to be built with the UWP toolchain")
endif ()

# Loading useful cmake functions

include(cmake/tweaks.cmake)

# Create a shared library that is compatible with the UWP runtime

add_library(${PROJECT_NAME} SHARED src/library.cpp)
set_tweaks_options(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE Shell32 Ole32 Kernel32)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR}/bin)
