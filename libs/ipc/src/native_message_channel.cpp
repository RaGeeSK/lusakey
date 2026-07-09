#include "lusakey/ipc/native_message_channel.h"

#include <stdexcept>

namespace lusakey::core::ipc {

namespace {

std::uint32_t decodeLengthLE(const unsigned char bytes[4]) {
    return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) | (static_cast<std::uint32_t>(bytes[3]) << 24);
}

void encodeLengthLE(std::uint32_t value, unsigned char out[4]) {
    out[0] = static_cast<unsigned char>(value & 0xFF);
    out[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
    out[2] = static_cast<unsigned char>((value >> 16) & 0xFF);
    out[3] = static_cast<unsigned char>((value >> 24) & 0xFF);
}

} // namespace

std::optional<std::string> readMessage(std::istream& in) {
    unsigned char lengthBytes[4];
    in.read(reinterpret_cast<char*>(lengthBytes), 4);
    if (in.gcount() == 0 && in.eof()) {
        return std::nullopt; // clean EOF between messages: the peer closed the pipe
    }
    if (!in) {
        throw std::runtime_error("native_message_channel: failed to read length prefix");
    }

    const std::uint32_t length = decodeLengthLE(lengthBytes);
    if (length > kMaxMessageBytes) {
        throw std::runtime_error("native_message_channel: message length exceeds the 1 MiB cap");
    }

    std::string message(length, '\0');
    if (length > 0) {
        in.read(message.data(), static_cast<std::streamsize>(length));
        if (!in) {
            throw std::runtime_error("native_message_channel: stream ended mid-message");
        }
    }
    return message;
}

void writeMessage(std::ostream& out, const std::string& message) {
    if (message.size() > kMaxMessageBytes) {
        throw std::runtime_error("native_message_channel: message exceeds the 1 MiB cap");
    }
    unsigned char lengthBytes[4];
    encodeLengthLE(static_cast<std::uint32_t>(message.size()), lengthBytes);
    out.write(reinterpret_cast<const char*>(lengthBytes), 4);
    out.write(message.data(), static_cast<std::streamsize>(message.size()));
    out.flush();
    if (!out) {
        throw std::runtime_error("native_message_channel: failed to write message");
    }
}

} // namespace lusakey::core::ipc
