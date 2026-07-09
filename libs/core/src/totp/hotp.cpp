#include "lusakey/core/totp/hotp.h"

#include <sodium.h>

#include <array>
#include <stdexcept>

#include "hmac_sha1.h"

namespace lusakey::core::totp {

namespace {

std::vector<std::uint8_t> computeHmac(HashAlgorithm algorithm,
                                       const std::vector<std::uint8_t>& secret,
                                       const std::array<std::uint8_t, 8>& counterBytes) {
    switch (algorithm) {
        case HashAlgorithm::SHA1: {
            const auto digest = internal::hmacSha1(secret.data(), secret.size(), counterBytes.data(),
                                                    counterBytes.size());
            return std::vector<std::uint8_t>(digest.begin(), digest.end());
        }
        case HashAlgorithm::SHA256: {
            std::array<std::uint8_t, crypto_auth_hmacsha256_BYTES> mac{};
            crypto_auth_hmacsha256_state state;
            crypto_auth_hmacsha256_init(&state, secret.data(), secret.size());
            crypto_auth_hmacsha256_update(&state, counterBytes.data(), counterBytes.size());
            crypto_auth_hmacsha256_final(&state, mac.data());
            return std::vector<std::uint8_t>(mac.begin(), mac.end());
        }
        case HashAlgorithm::SHA512: {
            std::array<std::uint8_t, crypto_auth_hmacsha512_BYTES> mac{};
            crypto_auth_hmacsha512_state state;
            crypto_auth_hmacsha512_init(&state, secret.data(), secret.size());
            crypto_auth_hmacsha512_update(&state, counterBytes.data(), counterBytes.size());
            crypto_auth_hmacsha512_final(&state, mac.data());
            return std::vector<std::uint8_t>(mac.begin(), mac.end());
        }
    }
    throw std::invalid_argument("computeHmac: unknown algorithm");
}

} // namespace

std::uint32_t hotp(const std::vector<std::uint8_t>& secret,
                    std::uint64_t counter,
                    unsigned int digits,
                    HashAlgorithm algorithm) {
    if (digits < 6 || digits > 8) {
        throw std::invalid_argument("hotp: digits must be between 6 and 8");
    }

    std::array<std::uint8_t, 8> counterBytes{};
    for (int i = 7; i >= 0; --i) {
        counterBytes[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(counter & 0xFF);
        counter >>= 8;
    }

    const auto hmac = computeHmac(algorithm, secret, counterBytes);

    // RFC 4226 §5.3 dynamic truncation.
    const std::uint8_t offset = hmac.back() & 0x0F;
    const std::uint32_t binCode = (static_cast<std::uint32_t>(hmac[offset] & 0x7F) << 24) |
                                   (static_cast<std::uint32_t>(hmac[offset + 1]) << 16) |
                                   (static_cast<std::uint32_t>(hmac[offset + 2]) << 8) |
                                   static_cast<std::uint32_t>(hmac[offset + 3]);

    static const std::uint32_t kPow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
    return binCode % kPow10[digits];
}

} // namespace lusakey::core::totp
