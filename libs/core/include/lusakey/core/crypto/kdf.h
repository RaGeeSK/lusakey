#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "lusakey/core/crypto/secure_bytes.h"

namespace lusakey::core::crypto {

inline constexpr std::size_t kSaltBytes = 16;       // crypto_pwhash_argon2id_SALTBYTES
inline constexpr std::size_t kDerivedKeyBytes = 32; // crypto_aead_xchacha20poly1305_ietf_KEYBYTES

// Argon2id cost parameters, persisted verbatim in the vault file header so a
// vault can always be opened with the parameters it was created with, even
// after this app's defaults change in a later version.
//
// Note: libsodium's crypto_pwhash_argon2id() always runs with a single lane
// (it does not expose a parallelism knob) — `parallelism` is stored for
// forward-compatibility/format-agility, not because it is currently tunable.
struct KdfParams {
    std::uint32_t memoryKiB = 262144; // 256 MiB
    std::uint32_t iterations = 3;
    std::uint32_t parallelism = 1;

    static KdfParams fast();     // 64 MiB / 2 iterations — quick unlock, weaker
    static KdfParams balanced();  // 256 MiB / 3 iterations — default
    static KdfParams strong();    // 512 MiB / 4 iterations — slower, stronger
};

// Derives a kDerivedKeyBytes-byte key from `password` and `salt` using
// Argon2id with the given cost parameters. Deterministic: the same inputs
// always yield the same key, which is what lets a vault be reopened later.
//
// Throws std::runtime_error if the underlying libsodium call fails (this
// happens in practice only if the process cannot allocate `memoryKiB` of
// memory for the KDF's working set).
SecureBuffer deriveKey(std::string_view password,
                       const std::uint8_t salt[kSaltBytes],
                       const KdfParams& params);

} // namespace lusakey::core::crypto
