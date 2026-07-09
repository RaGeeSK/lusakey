#include "lusakey/core/crypto/secure_bytes.h"

#include <sodium.h>

#include <new>
#include <utility>

namespace lusakey::core::crypto {

SecureBuffer::SecureBuffer(std::size_t size) : size_(size) {
    if (size_ == 0) {
        return;
    }
    data_ = static_cast<std::uint8_t*>(sodium_malloc(size_));
    if (!data_) {
        throw std::bad_alloc();
    }
    // Best-effort: may fail (e.g. RLIMIT_MEMLOCK too low on Linux); libsodium
    // does not report failure here, so there is nothing actionable to surface
    // to the caller. Swap exposure is a documented, accepted residual risk.
    sodium_mlock(data_, size_);
}

SecureBuffer::~SecureBuffer() {
    if (data_) {
        // sodium_munlock() zeroes the region before unlocking it.
        sodium_munlock(data_, size_);
        sodium_free(data_);
    }
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept
    : data_(std::exchange(other.data_, nullptr)), size_(std::exchange(other.size_, 0)) {}

SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept {
    if (this != &other) {
        if (data_) {
            sodium_munlock(data_, size_);
            sodium_free(data_);
        }
        data_ = std::exchange(other.data_, nullptr);
        size_ = std::exchange(other.size_, 0);
    }
    return *this;
}

void SecureBuffer::zero() noexcept {
    if (data_) {
        sodium_memzero(data_, size_);
    }
}

} // namespace lusakey::core::crypto
