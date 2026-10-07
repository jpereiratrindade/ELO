add_executable(elo-admin
    admin_http_server.hpp
    admin_http_server.cpp
    main.cpp
)

target_link_libraries(elo-admin PRIVATE
    elo_core
    Qt6::Core
    Qt6::Network
)

target_include_directories(elo-admin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

install(TARGETS elo-admin RUNTIME DESTINATION bin)
