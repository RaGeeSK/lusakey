#include "lusakey/core/crypto/random.h"

#include <sodium.h>

namespace lusakey::core::crypto {

void randomBytes(std::uint8_t* buffer, std::size_t size) {
    randombytes_buf(buffer, size);
}

std::vector<std::uint8_t> randomBytes(std::size_t size) {
    std::vector<std::uint8_t> buffer(size);
    randombytes_buf(buffer.data(), buffer.size());
    return buffer;
}

} // namespace lusakey::core::crypto
