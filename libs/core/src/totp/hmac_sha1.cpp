#include "hmac_sha1.h"

#include <algorithm>
#include <vector>

#include "sha1.h"

namespace lusakey::core::totp::internal {

namespace {
constexpr std::size_t kBlockSize = 64; // SHA-1 block size
}

std::array<std::uint8_t, 20> hmacSha1(const std::uint8_t* key, std::size_t keyLen,
                                       const std::uint8_t* message, std::size_t messageLen) {
    std::array<std::uint8_t, kBlockSize> keyBlock{};

    if (keyLen > kBlockSize) {
        const auto hashedKey = sha1(key, keyLen);
        std::copy(hashedKey.begin(), hashedKey.end(), keyBlock.begin());
    } else if (keyLen > 0) {
        std::copy(key, key + keyLen, keyBlock.begin());
    }

    std::array<std::uint8_t, kBlockSize> innerPad{};
    std::array<std::uint8_t, kBlockSize> outerPad{};
    for (std::size_t i = 0; i < kBlockSize; ++i) {
        innerPad[i] = static_cast<std::uint8_t>(keyBlock[i] ^ 0x36);
        outerPad[i] = static_cast<std::uint8_t>(keyBlock[i] ^ 0x5C);
    }

    std::vector<std::uint8_t> innerMessage;
    innerMessage.reserve(kBlockSize + messageLen);
    innerMessage.insert(innerMessage.end(), innerPad.begin(), innerPad.end());
    innerMessage.insert(innerMessage.end(), message, message + messageLen);
    const auto innerHash = sha1(innerMessage.data(), innerMessage.size());

    std::vector<std::uint8_t> outerMessage;
    outerMessage.reserve(kBlockSize + innerHash.size());
    outerMessage.insert(outerMessage.end(), outerPad.begin(), outerPad.end());
    outerMessage.insert(outerMessage.end(), innerHash.begin(), innerHash.end());

    return sha1(outerMessage.data(), outerMessage.size());
}

} // namespace lusakey::core::totp::internal
