find_package(Catch2 3 CONFIG REQUIRED)

add_executable(lusakey-ipc-tests
    test_native_message_channel.cpp
)

target_link_libraries(lusakey-ipc-tests PRIVATE lusakey-ipc Catch2::Catch2WithMain)

add_test(NAME lusakey-ipc-tests COMMAND lusakey-ipc-tests)
