add_executable(test_constitutional_invariants
    test_constitutional_invariants.cpp
)

target_link_libraries(test_constitutional_invariants PRIVATE
    elo_core
)

target_include_directories(test_constitutional_invariants PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_test(NAME ConstitutionalInvariants COMMAND test_constitutional_invariants)

add_executable(test_camera_catalog
    test_camera_catalog.cpp
)

target_link_libraries(test_camera_catalog PRIVATE
    elo_core
)

target_include_directories(test_camera_catalog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_test(NAME CameraCatalog COMMAND test_camera_catalog)

add_executable(test_sqlite_storage
    test_sqlite_storage.cpp
)

target_link_libraries(test_sqlite_storage PRIVATE
    elo_core
)

target_include_directories(test_sqlite_storage PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_test(NAME SqliteStorage COMMAND test_sqlite_storage)

add_executable(test_kiosk_presentation
    test_kiosk_presentation.cpp
)

target_link_libraries(test_kiosk_presentation PRIVATE
    elo_core
)

target_include_directories(test_kiosk_presentation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_test(NAME KioskPresentation COMMAND test_kiosk_presentation)

add_executable(test_content_system
    test_content_system.cpp
)

target_link_libraries(test_content_system PRIVATE
    elo_core
)

target_include_directories(test_content_system PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_test(NAME ContentSystem COMMAND test_content_system)

add_executable(test_content_publishing
    test_content_publishing.cpp
)

target_link_libraries(test_content_publishing PRIVATE
    elo_core
)

target_include_directories(test_content_publishing PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_test(NAME ContentPublishing COMMAND test_content_publishing)

