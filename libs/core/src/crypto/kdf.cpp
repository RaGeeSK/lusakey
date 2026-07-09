#include "lusakey/core/crypto/kdf.h"

#include <sodium.h>

#include <stdexcept>

namespace lusakey::core::crypto {

static_assert(kSaltBytes == crypto_pwhash_argon2id_SALTBYTES,
              "kSaltBytes must match libsodium's Argon2id salt size");
static_assert(kDerivedKeyBytes == crypto_aead_xchacha20poly1305_ietf_KEYBYTES,
              "kDerivedKeyBytes must match the AEAD key size it's used for");

KdfParams KdfParams::fast() { return KdfParams{65536, 2, 1}; }
KdfParams KdfParams::balanced() { return KdfParams{262144, 3, 1}; }
KdfParams KdfParams::strong() { return KdfParams{524288, 4, 1}; }

SecureBuffer deriveKey(std::string_view password,
                       const std::uint8_t salt[kSaltBytes],
                       const KdfParams& params) {
    SecureBuffer key(kDerivedKeyBytes);

    const int rc = crypto_pwhash_argon2id(
        key.data(), key.size(),
        password.data(), password.size(),
        salt,
        params.iterations,
        static_cast<std::size_t>(params.memoryKiB) * 1024,
        crypto_pwhash_argon2id_ALG_ARGON2ID13);

    if (rc != 0) {
        throw std::runtime_error(
            "Argon2id key derivation failed (likely insufficient memory for the "
            "requested cost — try a lower KDF profile)");
    }
    return key;
}

} // namespace lusakey::core::crypto
