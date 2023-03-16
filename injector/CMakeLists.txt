cmake_minimum_required(VERSION 3.11)
project(injector)

# Checking for MSVC toolchain with C++20 features

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
    message(FATAL_ERROR "MSVC toolchain is required!")
endif ()

# Loading useful cmake functions

include(cmake/embedded.cmake)

# Create an executable optimized for size

add_executable(${PROJECT_NAME} src/main.cpp)
set_embedded_options(${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR}/bin)
