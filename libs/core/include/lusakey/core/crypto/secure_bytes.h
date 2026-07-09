#pragma once

#include <cstddef>
#include <cstdint>

namespace lusakey::core::crypto {

// RAII buffer for sensitive data (derived keys, decrypted secrets).
//
// Backed by sodium_malloc: the allocation is placed with guard pages around it
// (catches over/underflow) and best-effort mlock'd to reduce the chance of the
// page being written to swap. The destructor zeroes the memory before freeing it.
//
// Not copyable (copying a secret defeats the point); movable.
class SecureBuffer {
public:
    explicit SecureBuffer(std::size_t size);
    ~SecureBuffer();

    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;

    SecureBuffer(SecureBuffer&& other) noexcept;
    SecureBuffer& operator=(SecureBuffer&& other) noexcept;

    std::uint8_t* data() noexcept { return data_; }
    const std::uint8_t* data() const noexcept { return data_; }
    std::size_t size() const noexcept { return size_; }

    // Explicitly zero the buffer's contents without releasing the allocation.
    void zero() noexcept;

private:
    std::uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
};

} // namespace lusakey::core::crypto
