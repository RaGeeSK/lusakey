# lusakey-ipc: native-messaging wire framing only (read/write length-prefixed
# JSON strings). No Qt, no JSON parsing — that's left to whatever links this
# (the future nmhost executable), keeping this library trivially small and
# dependency-free.

add_library(lusakey-ipc STATIC
    src/native_message_channel.cpp
)

target_include_directories(lusakey-ipc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(lusakey-ipc PUBLIC cxx_std_20)
