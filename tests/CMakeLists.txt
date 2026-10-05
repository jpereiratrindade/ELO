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
