find_package(Catch2 3 CONFIG REQUIRED)

add_executable(lusakey-qr-tests
    test_qr_decoder.cpp
)

target_link_libraries(lusakey-qr-tests PRIVATE lusakey-qr Catch2::Catch2WithMain)

add_test(NAME lusakey-qr-tests COMMAND lusakey-qr-tests)
