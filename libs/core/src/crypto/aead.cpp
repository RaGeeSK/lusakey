#include "lusakey/core/crypto/aead.h"

#include <sodium.h>

#include <stdexcept>

#include "lusakey/core/crypto/kdf.h"

namespace lusakey::core::crypto {

static_assert(kNonceBytes == crypto_aead_xchacha20poly1305_ietf_NPUBBYTES,
              "kNonceBytes must match libsodium's XChaCha20-Poly1305 nonce size");
static_assert(kTagBytes == crypto_aead_xchacha20poly1305_ietf_ABYTES,
              "kTagBytes must match libsodium's Poly1305 tag size");

std::vector<std::uint8_t> aeadEncrypt(const SecureBuffer& key,
                                       const std::uint8_t nonce[kNonceBytes],
                                       const std::vector<std::uint8_t>& aad,
                                       const std::vector<std::uint8_t>& plaintext) {
    if (key.size() != kDerivedKeyBytes) {
        throw std::invalid_argument("aeadEncrypt: key must be kDerivedKeyBytes long");
    }

    std::vector<std::uint8_t> ciphertext(plaintext.size() + kTagBytes);
    unsigned long long ciphertextLen = 0;

    const int rc = crypto_aead_xchacha20poly1305_ietf_encrypt(
        ciphertext.data(), &ciphertextLen,
        plaintext.data(), plaintext.size(),
        aad.empty() ? nullptr : aad.data(), aad.size(),
        nullptr, // nsec: unused by this construction
        nonce, key.data());

    if (rc != 0) {
        throw std::runtime_error("aeadEncrypt: encryption failed");
    }
    ciphertext.resize(static_cast<std::size_t>(ciphertextLen));
    return ciphertext;
}

std::vector<std::uint8_t> aeadDecrypt(const SecureBuffer& key,
                                       const std::uint8_t nonce[kNonceBytes],
                                       const std::vector<std::uint8_t>& aad,
                                       const std::vector<std::uint8_t>& ciphertext) {
    if (key.size() != kDerivedKeyBytes) {
        throw std::invalid_argument("aeadDecrypt: key must be kDerivedKeyBytes long");
    }
    if (ciphertext.size() < kTagBytes) {
        throw std::runtime_error("aeadDecrypt: ciphertext shorter than the authentication tag");
    }

    std::vector<std::uint8_t> plaintext(ciphertext.size() - kTagBytes);
    unsigned long long plaintextLen = 0;

    const int rc = crypto_aead_xchacha20poly1305_ietf_decrypt(
        plaintext.data(), &plaintextLen,
        nullptr, // nsec
        ciphertext.data(), ciphertext.size(),
        aad.empty() ? nullptr : aad.data(), aad.size(),
        nonce, key.data());

    if (rc != 0) {
        throw std::runtime_error(
            "aeadDecrypt: authentication failed (wrong password, or the file is "
            "corrupted/tampered)");
    }
    plaintext.resize(static_cast<std::size_t>(plaintextLen));
    return plaintext;
}

} // namespace lusakey::core::crypto
