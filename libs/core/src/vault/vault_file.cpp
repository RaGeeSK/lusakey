#include "lusakey/core/vault/vault_file.h"

#include <sodium.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <ios>
#include <iterator>
#include <system_error>

#include "lusakey/core/crypto/random.h"

namespace lusakey::core::vault {

namespace {

constexpr char kMagic[8] = {'L', 'U', 'S', 'A', 'K', 'E', 'Y', '\0'};
constexpr std::size_t kMagicSize = sizeof(kMagic);
constexpr std::uint16_t kCurrentVersion = 1;
constexpr std::uint8_t kKdfAlgArgon2id = 1;
constexpr std::uint8_t kAeadAlgXChaCha20Poly1305 = 1;
constexpr std::size_t kChecksumBytes = 32; // BLAKE2b-256, via crypto_generichash's default output size

// magic(8) + version(2) + kdfAlgId(1) + aeadAlgId(1) + memoryKiB(4) +
// iterations(4) + parallelism(4) + salt(16) + nonce(24) + ciphertextLen(4) = 68
constexpr std::size_t kHeaderSize =
    kMagicSize + 2 + 1 + 1 + 4 + 4 + 4 + crypto::kSaltBytes + crypto::kNonceBytes + 4;

void putU16LE(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void putU32LE(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

std::uint16_t getU16LE(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) | static_cast<std::uint16_t>(p[1] << 8);
}

std::uint32_t getU32LE(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

std::array<std::uint8_t, kChecksumBytes> checksumOf(const std::vector<std::uint8_t>& headerBytes,
                                                     const std::vector<std::uint8_t>& ciphertext) {
    crypto_generichash_state state;
    crypto_generichash_init(&state, nullptr, 0, kChecksumBytes);
    crypto_generichash_update(&state, headerBytes.data(), headerBytes.size());
    crypto_generichash_update(&state, ciphertext.data(), ciphertext.size());
    std::array<std::uint8_t, kChecksumBytes> out{};
    crypto_generichash_final(&state, out.data(), out.size());
    return out;
}

// Serializes the full fixed-size header, including ciphertextLen. This exact
// byte sequence is what's passed as AEAD associated data, so tampering with
// any header field (including the length) is caught at decrypt time.
std::vector<std::uint8_t> serializeHeader(const VaultFileHeader& header, std::uint32_t ciphertextLen) {
    std::vector<std::uint8_t> out;
    out.reserve(kHeaderSize);
    out.insert(out.end(), reinterpret_cast<const std::uint8_t*>(kMagic),
               reinterpret_cast<const std::uint8_t*>(kMagic) + kMagicSize);
    putU16LE(out, header.version);
    out.push_back(header.kdfAlgId);
    out.push_back(header.aeadAlgId);
    putU32LE(out, header.kdfParams.memoryKiB);
    putU32LE(out, header.kdfParams.iterations);
    putU32LE(out, header.kdfParams.parallelism);
    out.insert(out.end(), header.salt.begin(), header.salt.end());
    out.insert(out.end(), header.nonce.begin(), header.nonce.end());
    putU32LE(out, ciphertextLen);
    return out;
}

[[noreturn]] void fail(VaultFileError code, const char* message) {
    throw VaultFileException(code, message);
}

} // namespace

RawVaultFile readRaw(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        fail(VaultFileError::NotFound, "Vault file not found or not readable");
    }

    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
    if (file.bad()) {
        fail(VaultFileError::IoError, "Failed to read vault file");
    }

    if (bytes.size() < kHeaderSize + crypto::kTagBytes + kChecksumBytes) {
        fail(VaultFileError::Truncated, "Vault file is too small to be valid");
    }
    if (std::memcmp(bytes.data(), kMagic, kMagicSize) != 0) {
        fail(VaultFileError::BadMagic, "Not a lusakey vault file (bad magic bytes)");
    }

    RawVaultFile raw;
    std::size_t offset = kMagicSize;

    raw.header.version = getU16LE(&bytes[offset]);
    offset += 2;
    if (raw.header.version != kCurrentVersion) {
        fail(VaultFileError::UnsupportedVersion, "Unsupported vault file version");
    }

    raw.header.kdfAlgId = bytes[offset++];
    raw.header.aeadAlgId = bytes[offset++];

    raw.header.kdfParams.memoryKiB = getU32LE(&bytes[offset]);
    offset += 4;
    raw.header.kdfParams.iterations = getU32LE(&bytes[offset]);
    offset += 4;
    raw.header.kdfParams.parallelism = getU32LE(&bytes[offset]);
    offset += 4;

    std::copy(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
              bytes.begin() + static_cast<std::ptrdiff_t>(offset + crypto::kSaltBytes),
              raw.header.salt.begin());
    offset += crypto::kSaltBytes;

    std::copy(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
              bytes.begin() + static_cast<std::ptrdiff_t>(offset + crypto::kNonceBytes),
              raw.header.nonce.begin());
    offset += crypto::kNonceBytes;

    const std::uint32_t ciphertextLen = getU32LE(&bytes[offset]);
    offset += 4;

    if (offset != kHeaderSize) {
        fail(VaultFileError::IoError, "Internal header size mismatch");
    }

    const std::size_t expectedSize = kHeaderSize + static_cast<std::size_t>(ciphertextLen) + kChecksumBytes;
    if (bytes.size() != expectedSize) {
        fail(VaultFileError::Truncated, "Vault file size does not match its header");
    }

    raw.headerBytes.assign(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(kHeaderSize));
    raw.ciphertext.assign(bytes.begin() + static_cast<std::ptrdiff_t>(kHeaderSize),
                           bytes.begin() + static_cast<std::ptrdiff_t>(kHeaderSize + ciphertextLen));

    const std::vector<std::uint8_t> storedChecksum(bytes.end() - static_cast<std::ptrdiff_t>(kChecksumBytes),
                                                     bytes.end());
    const auto computed = checksumOf(raw.headerBytes, raw.ciphertext);
    if (!std::equal(computed.begin(), computed.end(), storedChecksum.begin())) {
        fail(VaultFileError::ChecksumMismatch,
             "Vault file failed its integrity checksum (corrupted or truncated)");
    }

    return raw;
}

std::vector<std::uint8_t> decryptPayload(const RawVaultFile& raw, const crypto::SecureBuffer& key) {
    try {
        return crypto::aeadDecrypt(key, raw.header.nonce.data(), raw.headerBytes, raw.ciphertext);
    } catch (const std::exception&) {
        fail(VaultFileError::AuthenticationFailed,
             "Wrong master password, or the vault file has been tampered with");
    }
}

std::vector<std::uint8_t> openAndDecrypt(const std::filesystem::path& path, std::string_view password) {
    const RawVaultFile raw = readRaw(path);
    crypto::SecureBuffer key = crypto::deriveKey(password, raw.header.salt.data(), raw.header.kdfParams);
    return decryptPayload(raw, key);
}

namespace {

// Shared tail end of every write path: fill in a fresh nonce, serialize the
// header (used verbatim as AAD), encrypt, checksum, and durably write via
// temp-file + atomic rename so a crash/power-loss mid-write cannot leave
// behind a half-written, unrecoverable vault file.
void writeVaultFile(const std::filesystem::path& path,
                     const crypto::SecureBuffer& key,
                     VaultFileHeader header,
                     const std::vector<std::uint8_t>& payload) {
    const auto nonceBytes = crypto::randomBytes(crypto::kNonceBytes);
    std::copy(nonceBytes.begin(), nonceBytes.end(), header.nonce.begin());

    // Ciphertext length is deterministic (plaintext length + tag) and known
    // before encrypting, so it can be baked into the header used as AAD.
    const auto headerBytes =
        serializeHeader(header, static_cast<std::uint32_t>(payload.size() + crypto::kTagBytes));

    const auto ciphertext = crypto::aeadEncrypt(key, header.nonce.data(), headerBytes, payload);
    const auto checksum = checksumOf(headerBytes, ciphertext);

    std::vector<std::uint8_t> fileBytes;
    fileBytes.reserve(headerBytes.size() + ciphertext.size() + checksum.size());
    fileBytes.insert(fileBytes.end(), headerBytes.begin(), headerBytes.end());
    fileBytes.insert(fileBytes.end(), ciphertext.begin(), ciphertext.end());
    fileBytes.insert(fileBytes.end(), checksum.begin(), checksum.end());

    auto tmpPath = path;
    tmpPath += ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            fail(VaultFileError::IoError, "Failed to open temporary file for writing");
        }
        out.write(reinterpret_cast<const char*>(fileBytes.data()),
                  static_cast<std::streamsize>(fileBytes.size()));
        if (!out) {
            fail(VaultFileError::IoError, "Failed to write vault file contents");
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, path, ec);
    if (ec) {
        fail(VaultFileError::IoError, "Failed to finalize vault file write (rename failed)");
    }
}

} // namespace

void writeNew(const std::filesystem::path& path,
              std::string_view password,
              const crypto::KdfParams& kdfParams,
              const std::vector<std::uint8_t>& payload) {
    VaultFileHeader header;
    header.version = kCurrentVersion;
    header.kdfAlgId = kKdfAlgArgon2id;
    header.aeadAlgId = kAeadAlgXChaCha20Poly1305;
    header.kdfParams = kdfParams;

    const auto saltBytes = crypto::randomBytes(crypto::kSaltBytes);
    std::copy(saltBytes.begin(), saltBytes.end(), header.salt.begin());

    crypto::SecureBuffer key = crypto::deriveKey(password, header.salt.data(), header.kdfParams);
    writeVaultFile(path, key, header, payload);
}

void writeWithKey(const std::filesystem::path& path,
                   const crypto::SecureBuffer& key,
                   const std::array<std::uint8_t, crypto::kSaltBytes>& salt,
                   const crypto::KdfParams& kdfParams,
                   const std::vector<std::uint8_t>& payload) {
    VaultFileHeader header;
    header.version = kCurrentVersion;
    header.kdfAlgId = kKdfAlgArgon2id;
    header.aeadAlgId = kAeadAlgXChaCha20Poly1305;
    header.kdfParams = kdfParams;
    header.salt = salt;
    writeVaultFile(path, key, header, payload);
}

} // namespace lusakey::core::vault
