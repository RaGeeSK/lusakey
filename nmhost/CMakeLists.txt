# Native-messaging host for the LusaKey browser extension (Chrome/Firefox).
# Reads framed JSON requests from stdin, dispatches them against VaultService,
# and writes framed JSON responses to stdout. See RequestDispatcher docs for
# the full action set. Excluded from the default build (LUSAKEY_BUILD_NMHOST=OFF);
# enable explicitly:
#   cmake --preset <preset> -DLUSAKEY_BUILD_NMHOST=ON

find_package(nlohmann_json CONFIG REQUIRED)

add_executable(lusakey-nmhost
    main.cpp
    src/request_dispatcher.cpp
    src/vault_path.cpp
)

target_link_libraries(lusakey-nmhost PRIVATE
    lusakey-core
    lusakey-ipc
    nlohmann_json::nlohmann_json
)

target_include_directories(lusakey-nmhost PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_features(lusakey-nmhost PRIVATE cxx_std_20)
