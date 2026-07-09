#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace lusakey::core::totp::internal {

// Minimal, self-contained SHA-1 (FIPS 180-4). Not part of the public API —
// used here ONLY as HMAC-SHA1's underlying hash for RFC 4226/6238
// compatibility (the overwhelming majority of real-world otpauth:// issuers
// still default to HMAC-SHA1). HMAC's security does not depend on the
// underlying hash's collision resistance, so this is fine for OTP despite
// SHA-1 being unsuitable elsewhere. libsodium deliberately omits SHA-1,
// hence vendoring it here rather than pulling in a second crypto library.
std::array<std::uint8_t, 20> sha1(const std::uint8_t* data, std::size_t len);

} // namespace lusakey::core::totp::internal
