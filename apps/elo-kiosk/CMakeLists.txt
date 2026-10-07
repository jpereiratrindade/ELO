add_executable(elo-kiosk
    camera_image_provider.cpp
    camera_image_provider.hpp
    main.cpp
    ../../ui/qml.qrc
)

target_compile_definitions(elo-kiosk PRIVATE
    ELO_FACE_DETECTOR_MODEL_PATH="${ELO_FACE_DETECTOR_MODEL_PATH}"
    ELO_FACE_RECOGNIZER_MODEL_PATH="${ELO_FACE_RECOGNIZER_MODEL_PATH}"
)

target_link_libraries(elo-kiosk PRIVATE
    elo_core
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

target_include_directories(elo-kiosk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

install(TARGETS elo-kiosk RUNTIME DESTINATION bin)
