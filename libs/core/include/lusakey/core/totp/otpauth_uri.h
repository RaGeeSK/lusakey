#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "lusakey/core/totp/totp.h"

namespace lusakey::core::totp {

// A parsed `otpauth://totp/...` enrollment URI — the de facto standard
// shared by Google Authenticator, Authy, and virtually every 2FA issuer's QR
// code. `otpauth://hotp/...` (counter-based) is out of scope: TOTP covers
// the overwhelming majority of real-world QR codes users will import.
struct OtpAuthUri {
    std::string label;                // e.g. "Example:alice@example.com"
    std::string issuer;                // e.g. "Example"; falls back to the label's "Issuer:" prefix
    std::vector<std::uint8_t> secret;  // raw decoded secret bytes (NOT Base32 text)
    TotpParams params;
};

// Parses a `otpauth://totp/...` URI. Throws std::invalid_argument if the
// scheme isn't `otpauth://totp/`, the `secret` parameter is missing/empty,
// or the secret isn't valid Base32.
OtpAuthUri parseOtpAuthUri(std::string_view uri);

// Serializes back to a `otpauth://totp/...` URI (round-trip tests, or a
// "show as text" affordance in the UI for manual re-entry elsewhere).
std::string toOtpAuthUri(const OtpAuthUri& entry);

} // namespace lusakey::core::totp
