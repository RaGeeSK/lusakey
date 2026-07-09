#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "lusakey/core/crypto/aead.h"
#include "lusakey/core/crypto/kdf.h"
#include "lusakey/core/crypto/secure_bytes.h"

namespace lusakey::core::vault {

enum class VaultFileError {
    NotFound,
    IoError,
    BadMagic,
    UnsupportedVersion,
    Truncated,
    ChecksumMismatch,
    AuthenticationFailed,
};

class VaultFileException : public std::runtime_error {
public:
    VaultFileException(VaultFileError code, const std::string& message)
        : std::runtime_error(message), code_(code) {}

    VaultFileError code() const noexcept { return code_; }

private:
    VaultFileError code_;
};

struct VaultFileHeader {
    std::uint16_t version = 1;
    std::uint8_t kdfAlgId = 1;  // 1 = Argon2id
    std::uint8_t aeadAlgId = 1; // 1 = XChaCha20-Poly1305-IETF
    crypto::KdfParams kdfParams;
    std::array<std::uint8_t, crypto::kSaltBytes> salt{};
    std::array<std::uint8_t, crypto::kNonceBytes> nonce{};
};

// A vault file that has been read and structurally validated (magic bytes,
// supported version, integrity checksum) but NOT decrypted — producing this
// requires no password, which is what lets callers report "this file is
// corrupted" separately from "wrong password" instead of conflating both
// into a single failed decrypt attempt.
struct RawVaultFile {
    VaultFileHeader header;
    std::vector<std::uint8_t> headerBytes; // exact serialized header; used verbatim as the AEAD's AAD
    std::vector<std::uint8_t> ciphertext;  // payload || 16-byte Poly1305 tag
};

// Reads `path` and validates magic bytes, format version, and the integrity
// checksum. Throws VaultFileException on any structural problem. Does not
// require a password and does not attempt decryption.
RawVaultFile readRaw(const std::filesystem::path& path);

// Decrypts an already-read vault file's payload using a key derived via
// crypto::deriveKey(password, raw.header.salt.data(), raw.header.kdfParams).
// Throws VaultFileException(AuthenticationFailed) on wrong key or tampering
// (the two are indistinguishable by design).
std::vector<std::uint8_t> decryptPayload(const RawVaultFile& raw, const crypto::SecureBuffer& key);

// Convenience "unlock" path: reads the file, derives the key from `password`
// using the file's own stored KDF parameters, and decrypts.
std::vector<std::uint8_t> openAndDecrypt(const std::filesystem::path& path, std::string_view password);

// Encrypts `payload` under a freshly derived key (new random salt) and a
// fresh random nonce, and durably writes it to `path` (write-to-temp-file +
// atomic rename, so a crash mid-write cannot corrupt an existing vault).
// Runs the (deliberately slow) Argon2id KDF — use this only for vault
// creation and master-password changes, NOT for routine saves.
void writeNew(const std::filesystem::path& path,
              std::string_view password,
              const crypto::KdfParams& kdfParams,
              const std::vector<std::uint8_t>& payload);

// Encrypts `payload` under an ALREADY-DERIVED `key` and the given `salt`/
// `kdfParams` (persisted verbatim in the header, unchanged, so the vault
// stays openable by the same password) with a freshly generated nonce, and
// durably writes it to `path`. This is the routine "save" path: it does NOT
// run the KDF, which is what makes it cheap enough to call after every
// single edit — the KDF only runs once per unlock/create/re-key.
void writeWithKey(const std::filesystem::path& path,
                   const crypto::SecureBuffer& key,
                   const std::array<std::uint8_t, crypto::kSaltBytes>& salt,
                   const crypto::KdfParams& kdfParams,
                   const std::vector<std::uint8_t>& payload);

} // namespace lusakey::core::vault
