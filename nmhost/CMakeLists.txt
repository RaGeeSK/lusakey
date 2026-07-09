# Stub native-messaging host: proves the VaultService + native-messaging
# framing seam (see AGENTS.md, M9) compiles and links end to end, with NO Qt
# dependency. Not wired up to a real browser extension yet — no manifest
# registration, no real request router, just a toy "ping" action. Excluded
# from the default build (LUSAKEY_BUILD_NMHOST=OFF); enable explicitly:
#   cmake --preset <preset> -DLUSAKEY_BUILD_NMHOST=ON

find_package(nlohmann_json CONFIG REQUIRED)

add_executable(lusakey-nmhost main.cpp)

target_link_libraries(lusakey-nmhost PRIVATE
    lusakey-core
    lusakey-ipc
    nlohmann_json::nlohmann_json
)

target_compile_features(lusakey-nmhost PRIVATE cxx_std_20)
