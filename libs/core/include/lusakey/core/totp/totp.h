#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "lusakey/core/totp/hotp.h"

namespace lusakey::core::totp {

struct TotpParams {
    HashAlgorithm algorithm = HashAlgorithm::SHA1;
    unsigned int digits = 6;
    unsigned int periodSeconds = 30;
};

// RFC 6238: HOTP evaluated at counter = floor(unixTime / periodSeconds).
// `now` is an explicit parameter (defaulting to the real clock) so callers —
// and, importantly, tests against RFC 6238's published vectors — don't
// depend on wall-clock time.
std::uint32_t totp(const std::vector<std::uint8_t>& secret,
                    const TotpParams& params,
                    std::chrono::system_clock::time_point now = std::chrono::system_clock::now());

// Seconds remaining in the current period — drives a UI countdown ring.
// Correct even if queried after the app was backgrounded/suspended, since it
// derives from wall-clock time rather than decrementing a stored counter.
unsigned int secondsRemaining(const TotpParams& params,
                               std::chrono::system_clock::time_point now = std::chrono::system_clock::now());

// Zero-padded decimal rendering of a code, e.g. formatCode(83, 6) == "000083".
std::string formatCode(std::uint32_t code, unsigned int digits);

} // namespace lusakey::core::totp
