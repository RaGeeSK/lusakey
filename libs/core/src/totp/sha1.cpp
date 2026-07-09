#include "sha1.h"

#include <vector>

namespace lusakey::core::totp::internal {

namespace {

std::uint32_t rotl32(std::uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

void processBlock(std::uint32_t h[5], const std::uint8_t block[64]) {
    std::uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24) |
               (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
               static_cast<std::uint32_t>(block[i * 4 + 3]);
    }
    for (int i = 16; i < 80; ++i) {
        w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    std::uint32_t a = h[0];
    std::uint32_t b = h[1];
    std::uint32_t c = h[2];
    std::uint32_t d = h[3];
    std::uint32_t e = h[4];

    for (int i = 0; i < 80; ++i) {
        std::uint32_t f;
        std::uint32_t k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999u;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }
        const std::uint32_t temp = rotl32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotl32(b, 30);
        b = a;
        a = temp;
    }

    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

} // namespace

std::array<std::uint8_t, 20> sha1(const std::uint8_t* data, std::size_t len) {
    std::uint32_t h[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};

    const std::uint64_t bitLen = static_cast<std::uint64_t>(len) * 8;

    std::vector<std::uint8_t> message(data, data + len);
    message.push_back(0x80);
    while (message.size() % 64 != 56) {
        message.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        message.push_back(static_cast<std::uint8_t>((bitLen >> (i * 8)) & 0xFF));
    }

    for (std::size_t offset = 0; offset < message.size(); offset += 64) {
        processBlock(h, &message[offset]);
    }

    std::array<std::uint8_t, 20> digest{};
    for (int i = 0; i < 5; ++i) {
        digest[static_cast<std::size_t>(i) * 4 + 0] = static_cast<std::uint8_t>((h[i] >> 24) & 0xFF);
        digest[static_cast<std::size_t>(i) * 4 + 1] = static_cast<std::uint8_t>((h[i] >> 16) & 0xFF);
        digest[static_cast<std::size_t>(i) * 4 + 2] = static_cast<std::uint8_t>((h[i] >> 8) & 0xFF);
        digest[static_cast<std::size_t>(i) * 4 + 3] = static_cast<std::uint8_t>(h[i] & 0xFF);
    }
    return digest;
}

} // namespace lusakey::core::totp::internal
