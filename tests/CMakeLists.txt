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
