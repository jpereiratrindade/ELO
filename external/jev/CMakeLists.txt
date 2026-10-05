cmake_minimum_required(VERSION 3.25)
project(jev_contract LANGUAGES CXX)

add_library(jev_contract INTERFACE)

target_include_directories(jev_contract INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(jev_contract INTERFACE cxx_std_26)
