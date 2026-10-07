cmake_minimum_required(VERSION 3.25)
project(elo VERSION 0.2.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and options
add_compile_options(-Wall -Wextra -Wpedantic)

# Enable Qt moc/rcc/uic automation
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Discover Qt 6, OpenCV, SQLite3
find_package(Qt6 6.5 COMPONENTS Core Gui Quick Qml Network REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc objdetect videoio dnn)
find_package(SQLite3 REQUIRED)
if(TARGET SQLite3::SQLite3)
    set(ELO_SQLITE_TARGET SQLite3::SQLite3)
elseif(TARGET SQLite::SQLite3)
    set(ELO_SQLITE_TARGET SQLite::SQLite3)
else()
    add_library(SQLite3::SQLite3 UNKNOWN IMPORTED)
    set_target_properties(SQLite3::SQLite3 PROPERTIES
        IMPORTED_LOCATION "${SQLite3_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIRS}"
    )
    set(ELO_SQLITE_TARGET SQLite3::SQLite3)
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBCAMERA QUIET IMPORTED_TARGET libcamera)
endif()

option(ELO_DOWNLOAD_VISION_MODELS "Download pinned YuNet/SFace models during configure" ON)
include(EloVisionModels)
elo_resolve_vision_model(
    ELO_FACE_DETECTOR_MODEL_PATH
    ELO_FACE_DETECTOR_MODEL
    face_detection_yunet_2023mar.onnx
    https://media.githubusercontent.com/media/opencv/opencv_zoo/main/models/face_detection_yunet/face_detection_yunet_2023mar.onnx
    8f2383e4dd3cfbb4553ea8718107fc0423210dc964f9f4280604804ed2552fa4
)
elo_resolve_vision_model(
    ELO_FACE_RECOGNIZER_MODEL_PATH
    ELO_FACE_RECOGNIZER_MODEL
    face_recognition_sface_2021dec_int8.onnx
    https://media.githubusercontent.com/media/opencv/opencv_zoo/main/models/face_recognition_sface/face_recognition_sface_2021dec_int8.onnx
    2b0e941e6f16cc048c20aee0c8e31f569118f65d702914540f7bfdc14048d78a
)

# Consumed components (Constitutional Invariants E2, E3)
add_subdirectory(external/ente-kernel)
add_subdirectory(external/jev)

# Core ELO Library
add_library(elo_core
    include/elo/perception/vision_service.hpp
    include/elo/system/control_socket.hpp
    include/elo/ui/kiosk_presentation_model.hpp
    src/biometric/biometric_matcher.cpp
    src/content/content_bundle.cpp
    src/content/content_catalog.cpp
    src/content/content_validator.cpp
    src/content/content_selector.cpp
    src/content/recipe_executor.cpp
    src/experience/experience_engine.cpp
    src/perception/camera_catalog.cpp
    src/perception/vision_service.cpp
    src/storage/sqlite_storage.cpp
    src/system/control_socket.cpp
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
    Qt6::Network
    ${OpenCV_LIBS}
    ${ELO_SQLITE_TARGET}
)

target_compile_definitions(elo_core PUBLIC ELO_HAS_QT)

if(TARGET PkgConfig::LIBCAMERA)
    target_link_libraries(elo_core PRIVATE PkgConfig::LIBCAMERA)
    target_compile_definitions(elo_core PRIVATE ELO_HAS_LIBCAMERA)
    message(STATUS "ELO camera discovery backend: libcamera")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "ELO camera discovery backend: V4L2")
else()
    message(STATUS "ELO camera discovery backend: unavailable")
endif()

# Applications
add_subdirectory(apps/elo-kiosk)
add_subdirectory(apps/elo-admin)

# Installation of Web Admin Studio & Documentation
install(DIRECTORY web/admin DESTINATION share/elo)
install(DIRECTORY docs DESTINATION share/doc/elo)

# Constitutional Tests and Experiments
enable_testing()
add_subdirectory(tests)
