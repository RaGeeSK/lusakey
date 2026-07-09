#include "base32.h"

#include <stdexcept>

namespace lusakey::core::totp::internal {

namespace {

constexpr char kAlphabet[32] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                                 '2', '3', '4', '5', '6', '7'};

int decodeChar(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '2' && c <= '7') return c - '2' + 26;
    return -1;
}

} // namespace

std::vector<std::uint8_t> base32Decode(std::string_view input) {
    std::vector<std::uint8_t> out;
    std::uint64_t buffer = 0;
    int bitsInBuffer = 0;

    for (const char c : input) {
        if (c == '=' || c == ' ' || c == '-') {
            continue; // padding / user-pasted formatting noise
        }
        const int value = decodeChar(c);
        if (value < 0) {
            throw std::invalid_argument("base32Decode: invalid character in input");
        }
        buffer = (buffer << 5) | static_cast<std::uint64_t>(value);
        bitsInBuffer += 5;
        if (bitsInBuffer >= 8) {
            bitsInBuffer -= 8;
            out.push_back(static_cast<std::uint8_t>((buffer >> bitsInBuffer) & 0xFF));
        }
    }
    return out;
}

std::string base32Encode(const std::vector<std::uint8_t>& data) {
    std::string out;
    std::uint64_t buffer = 0;
    int bitsInBuffer = 0;

    for (const std::uint8_t byte : data) {
        buffer = (buffer << 8) | byte;
        bitsInBuffer += 8;
        while (bitsInBuffer >= 5) {
            bitsInBuffer -= 5;
            out.push_back(kAlphabet[(buffer >> bitsInBuffer) & 0x1F]);
        }
    }
    if (bitsInBuffer > 0) {
        out.push_back(kAlphabet[(buffer << (5 - bitsInBuffer)) & 0x1F]);
    }
    while (out.size() % 8 != 0) {
        out.push_back('=');
    }
    return out;
}

} // namespace lusakey::core::totp::internal
