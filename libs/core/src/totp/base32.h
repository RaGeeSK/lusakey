#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace lusakey::core::totp::internal {

// RFC 4648 Base32 — the encoding used by otpauth:// secrets. Decoding is
// case-insensitive and tolerates '=' padding, spaces, and hyphens (real-world
// otpauth secrets are hand copy-pasted by users). Throws std::invalid_argument
// on a genuinely invalid character.
std::vector<std::uint8_t> base32Decode(std::string_view input);

// Encodes with the standard RFC 4648 alphabet and '=' padding.
std::string base32Encode(const std::vector<std::uint8_t>& data);

} // namespace lusakey::core::totp::internal
