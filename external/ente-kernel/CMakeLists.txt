cmake_minimum_required(VERSION 3.25)
project(ente_kernel_consumed LANGUAGES CXX)

add_library(ente_kernel STATIC
    kernel.cpp
)

target_include_directories(ente_kernel PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set_target_properties(ente_kernel PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)
