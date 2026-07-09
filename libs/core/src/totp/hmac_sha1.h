#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace lusakey::core::totp::internal {

// HMAC-SHA1 (RFC 2104), built on the vendored sha1() above. Used by hotp.cpp
// when HashAlgorithm::SHA1 is requested.
std::array<std::uint8_t, 20> hmacSha1(const std::uint8_t* key, std::size_t keyLen,
                                       const std::uint8_t* message, std::size_t messageLen);

} // namespace lusakey::core::totp::internal
