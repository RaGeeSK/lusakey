find_package(Catch2 3 CONFIG REQUIRED)

add_executable(lusakey-nmhost-logic-tests
    test_request_dispatcher.cpp
    test_vault_path.cpp
)

target_link_libraries(lusakey-nmhost-logic-tests PRIVATE lusakey-nmhost-logic Catch2::Catch2WithMain)

add_test(NAME lusakey-nmhost-logic-tests COMMAND lusakey-nmhost-logic-tests)
