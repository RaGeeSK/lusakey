# Native-messaging host: lets a future browser extension talk to the local
# vault via a small stdin/stdout JSON protocol (see
# lusakey-nmhost-logic/include/lusakey/nmhost/request_dispatcher.h). No Qt
# dependency, same as libs/core.
#
# The dispatch logic is its own static library, built unconditionally (like
# lusakey-ipc), so tests/nmhost-logic can exercise it without needing the
# final executable enabled. Only the executable itself — which nothing else
# links against and which isn't installed/shipped anywhere yet — stays
# behind LUSAKEY_BUILD_NMHOST:
#   cmake --preset <preset> -DLUSAKEY_BUILD_NMHOST=ON

find_package(nlohmann_json CONFIG REQUIRED)

add_library(lusakey-nmhost-logic STATIC
    src/request_dispatcher.cpp
    src/vault_path.cpp
)
target_include_directories(lusakey-nmhost-logic PUBLIC include)
target_link_libraries(lusakey-nmhost-logic PUBLIC lusakey-core nlohmann_json::nlohmann_json)
target_compile_features(lusakey-nmhost-logic PUBLIC cxx_std_20)

if(LUSAKEY_BUILD_NMHOST)
    add_executable(lusakey-nmhost main.cpp)
    target_link_libraries(lusakey-nmhost PRIVATE lusakey-nmhost-logic lusakey-ipc)
    target_compile_features(lusakey-nmhost PRIVATE cxx_std_20)
endif()
