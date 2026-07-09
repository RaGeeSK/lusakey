find_package(Catch2 3 CONFIG REQUIRED)

add_executable(lusakey-core-tests
    test_crypto.cpp
    test_vault_file.cpp
    test_totp.cpp
    test_vault_service.cpp
)

target_link_libraries(lusakey-core-tests PRIVATE lusakey-core Catch2::Catch2WithMain)

# A single ctest entry running the whole Catch2 binary. Deliberately not using
# Catch2's CMake module (catch_discover_tests) for per-case granularity, to
# keep the build's CMake surface small; run the binary directly with `-l`/tag
# filters for finer-grained runs during local development.
add_test(NAME lusakey-core-tests COMMAND lusakey-core-tests)
