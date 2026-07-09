# lusakey-core: crypto, vault file format, TOTP, business logic.
# Deliberately has ZERO dependency on Qt — this is what lets the future
# native-messaging-host (browser extension backend) reuse it without a rewrite.

find_package(unofficial-sodium CONFIG REQUIRED)

add_library(lusakey-core STATIC
    src/crypto/secure_bytes.cpp
    src/crypto/random.cpp
    src/crypto/kdf.cpp
    src/crypto/aead.cpp
    src/vault/vault_file.cpp
    src/vault/vault_model.cpp
    src/vault/vault_service.cpp
    src/totp/sha1.cpp
    src/totp/hmac_sha1.cpp
    src/totp/base32.cpp
    src/totp/hotp.cpp
    src/totp/totp.cpp
    src/totp/otpauth_uri.cpp
    src/util/password_generator.cpp
)

target_include_directories(lusakey-core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# vcpkg's libsodium port exports this (unusual-looking but correct) target name.
target_link_libraries(lusakey-core PUBLIC unofficial-sodium::sodium)

target_compile_features(lusakey-core PUBLIC cxx_std_20)
