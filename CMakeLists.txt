cmake_minimum_required(VERSION 3.25)
project(elo VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and options
add_compile_options(-Wall -Wextra -Wpedantic)

# Enable Qt moc/rcc/uic automation
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Discover Qt 6
find_package(Qt6 6.5 COMPONENTS Core Gui Quick Qml REQUIRED)

# Consumed components (Constitutional Invariants E2, E3)
add_subdirectory(external/ente-kernel)
add_subdirectory(external/jev)

# Core ELO Library
add_library(elo_core
    include/elo/ui/kiosk_presentation_model.hpp
    src/biometric/biometric_matcher.cpp
    src/experience/experience_engine.cpp
    src/ui/kiosk_presentation_model.cpp
)

target_include_directories(elo_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/ente-kernel>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/jev/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(elo_core PUBLIC
    ente_kernel
    jev_contract
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

target_compile_definitions(elo_core PUBLIC ELO_HAS_QT)

# Applications
add_subdirectory(apps/elo-kiosk)

# Constitutional Tests and Experiments
enable_testing()
add_subdirectory(tests)
