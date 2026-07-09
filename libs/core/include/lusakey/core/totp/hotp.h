#pragma once

#include <cstdint>
#include <vector>

namespace lusakey::core::totp {

enum class HashAlgorithm { SHA1, SHA256, SHA512 };

// RFC 4226 HOTP: computes a `digits`-digit (6..8) one-time code from `secret`
// (raw decoded bytes, NOT Base32 text) and a `counter` value.
//
// SHA1 — the default, and what the overwhelming majority of real-world
// otpauth:// issuers (Google/GitHub/AWS, ...) use — is implemented via a
// small vendored HMAC-SHA1/SHA-1 (libsodium deliberately omits SHA-1).
// SHA256/SHA512 use libsodium's crypto_auth_hmacsha256/512 directly.
//
// Throws std::invalid_argument if `digits` is outside [6, 8].
std::uint32_t hotp(const std::vector<std::uint8_t>& secret,
                    std::uint64_t counter,
                    unsigned int digits = 6,
                    HashAlgorithm algorithm = HashAlgorithm::SHA1);

} // namespace lusakey::core::totp
