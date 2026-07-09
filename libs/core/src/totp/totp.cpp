#include "lusakey/core/totp/totp.h"

#include <iomanip>
#include <sstream>

namespace lusakey::core::totp {

std::uint32_t totp(const std::vector<std::uint8_t>& secret,
                    const TotpParams& params,
                    std::chrono::system_clock::time_point now) {
    const auto unixSeconds =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const auto counter = static_cast<std::uint64_t>(unixSeconds) / params.periodSeconds;
    return hotp(secret, counter, params.digits, params.algorithm);
}

unsigned int secondsRemaining(const TotpParams& params, std::chrono::system_clock::time_point now) {
    const auto unixSeconds =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const auto intoPeriod = static_cast<unsigned int>(unixSeconds % params.periodSeconds);
    return params.periodSeconds - intoPeriod;
}

std::string formatCode(std::uint32_t code, unsigned int digits) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(static_cast<int>(digits)) << code;
    return oss.str();
}

} // namespace lusakey::core::totp
