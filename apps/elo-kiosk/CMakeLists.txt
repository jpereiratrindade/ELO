add_executable(elo-kiosk
    main.cpp
    ../../ui/qml.qrc
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
