#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "lusakey/core/crypto/secure_bytes.h"

namespace lusakey::core::crypto {

inline constexpr std::size_t kNonceBytes = 24; // crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
inline constexpr std::size_t kTagBytes = 16;   // crypto_aead_xchacha20poly1305_ietf_ABYTES

// Encrypts `plaintext` with `key` (must be kDerivedKeyBytes long) and a
// caller-supplied `nonce`. `aad` is authenticated but not encrypted — used to
// bind the vault file header to the ciphertext so tampering with the header
// (KDF params, salt, nonce) is detected at decrypt time.
//
// The nonce MUST be freshly random for every encryption performed with a
// given key (this is why the vault format generates a new nonce on every
// save rather than reusing/incrementing one).
//
// Returns ciphertext || 16-byte Poly1305 tag, concatenated.
std::vector<std::uint8_t> aeadEncrypt(const SecureBuffer& key,
                                       const std::uint8_t nonce[kNonceBytes],
                                       const std::vector<std::uint8_t>& aad,
                                       const std::vector<std::uint8_t>& plaintext);

// Decrypts and verifies `ciphertext` (as produced by aeadEncrypt: payload || tag).
// Throws std::runtime_error if authentication fails — this is the single
// signal for "wrong password" and "tampered/corrupted ciphertext or aad";
// the two cannot be distinguished by design (an oracle that could tell them
// apart would leak information to an attacker).
std::vector<std::uint8_t> aeadDecrypt(const SecureBuffer& key,
                                       const std::uint8_t nonce[kNonceBytes],
                                       const std::vector<std::uint8_t>& aad,
                                       const std::vector<std::uint8_t>& ciphertext);

} // namespace lusakey::core::crypto
