#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace lusakey::core::crypto {

// Fills `size` bytes at `buffer` using the OS CSPRNG (BCryptGenRandom /
// getrandom / arc4random under the hood). Use this — never std::rand,
// std::mt19937, or Qt's qrand — for anything security-relevant: salts,
// nonces, and the password generator feature.
void randomBytes(std::uint8_t* buffer, std::size_t size);

// Convenience overload returning a freshly allocated vector.
std::vector<std::uint8_t> randomBytes(std::size_t size);

} // namespace lusakey::core::crypto
