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

// One "key slot": a KDF-derived key that wraps (AEAD-encrypts) the vault's
// actual body-encryption key (the DEK — Data Encryption Key, a random value
// generated once at vault creation, never itself derived from a password).
// A vault always has a password slot; optionally also a recovery slot
// (derived from secret-question answers). Multiple slots let more than one
// secret unlock the same vault without re-encrypting the body per slot —
// the same "key-wrapping" pattern LUKS/BitLocker use for multiple
// passphrases/recovery keys.
struct KeySlot {
    crypto::KdfParams kdfParams;
    std::array<std::uint8_t, crypto::kSaltBytes> salt{};
    std::array<std::uint8_t, crypto::kNonceBytes> nonce{}; // wraps the DEK
    std::array<std::uint8_t, crypto::kDerivedKeyBytes + crypto::kTagBytes> wrappedDek{};
};

struct VaultFileHeader {
    std::uint16_t version = 2;
    std::uint8_t kdfAlgId = 1;  // 1 = Argon2id
    std::uint8_t aeadAlgId = 1; // 1 = XChaCha20-Poly1305-IETF

    KeySlot passwordSlot;

    bool recoveryEnabled = false;
    std::vector<std::string> recoveryQuestions; // plaintext prompts — not secret, shown before answers are entered
    KeySlot recoverySlot;                        // only meaningful if recoveryEnabled

    std::array<std::uint8_t, crypto::kNonceBytes> bodyNonce{};
};

// A vault file that has been read and structurally validated (magic bytes,
// supported version, integrity checksum) but with no key slot unwrapped —
// producing this requires no password/answers, letting callers report
// "this file is corrupted" separately from "wrong password/answers".
struct RawVaultFile {
    VaultFileHeader header;
    std::vector<std::uint8_t> headerBytes; // exact serialized header; used verbatim as the body AEAD's AAD
    std::vector<std::uint8_t> ciphertext;  // body payload || 16-byte tag, encrypted under the DEK
};

// Reads `path` and validates magic bytes, format version, and the integrity
// checksum. Throws VaultFileException on any structural problem. Does not
// require a password/answers and does not attempt to unwrap any key slot.
RawVaultFile readRaw(const std::filesystem::path& path);

// Derives a KEK from `secret` (a master password, or normalized/joined
// recovery answers — the caller is responsible for normalizing) using
// `slot`'s own kdfParams/salt, and uses it to unwrap (AEAD-decrypt)
// `slot.wrappedDek`. Throws VaultFileException(AuthenticationFailed) if
// `secret` is wrong for this slot.
crypto::SecureBuffer unwrapDek(const KeySlot& slot, std::string_view secret);

// Builds a fresh KeySlot for `dek`: a new random salt, a KEK derived from
// `secret`, and `dek` wrapped under it with a fresh nonce. Used for both the
// password slot (creation, change-password) and the recovery slot
// (configuring/changing recovery questions) — the operation is identical.
KeySlot makeKeySlot(const crypto::SecureBuffer& dek, std::string_view secret, const crypto::KdfParams& kdfParams);

// Decrypts the body of an already-read vault file with an already-unwrapped
// DEK. Throws VaultFileException(AuthenticationFailed) on tampering/corruption.
std::vector<std::uint8_t> decryptBody(const RawVaultFile& raw, const crypto::SecureBuffer& dek);

// Writes (or re-writes) a complete vault file: encrypts `payload` under
// `dek` with a fresh body nonce, and durably writes to `path` (temp file +
// atomic rename, so a crash mid-write cannot corrupt an existing vault).
// Runs NO KDF — slots are passed in already-built (from makeKeySlot at
// creation/re-key/recovery setup, or carried forward unchanged from an
// existing RawVaultFile's header on a routine save). `recoverySlot`'s
// contents are ignored when `recoveryEnabled` is false.
void writeVault(const std::filesystem::path& path,
                 const crypto::SecureBuffer& dek,
                 const KeySlot& passwordSlot,
                 bool recoveryEnabled,
                 const std::vector<std::string>& recoveryQuestions,
                 const KeySlot& recoverySlot,
                 const std::vector<std::uint8_t>& payload);

// Convenience: creates a brand-new vault — a fresh random DEK, a password
// slot derived from `password`, no recovery slot — and writes it.
void createNew(const std::filesystem::path& path,
               std::string_view password,
               const crypto::KdfParams& kdfParams,
               const std::vector<std::uint8_t>& payload);

} // namespace lusakey::core::vault
