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
constexpr std::uint16_t kCurrentVersion = 2;
constexpr std::uint8_t kKdfAlgArgon2id = 1;
constexpr std::uint8_t kAeadAlgXChaCha20Poly1305 = 1;
constexpr std::size_t kChecksumBytes = 32; // BLAKE2b-256
constexpr std::size_t kWrappedDekBytes = crypto::kDerivedKeyBytes + crypto::kTagBytes; // 48
constexpr std::size_t kMaxRecoveryQuestions = 20; // sanity cap, not a real-world limit
constexpr std::size_t kMaxQuestionBytes = 2000;   // defense-in-depth cap

// --- Small binary writer/reader, mirroring the pattern used in
// vault_model.cpp — kept independent (not shared) so this file's on-disk
// format and the payload's in-memory format can evolve separately. ---

class Writer {
public:
    void u8(std::uint8_t v) { out_.push_back(v); }
    void u16(std::uint16_t v) {
        out_.push_back(static_cast<std::uint8_t>(v & 0xFF));
        out_.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    }
    void u32(std::uint32_t v) {
        out_.push_back(static_cast<std::uint8_t>(v & 0xFF));
        out_.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out_.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        out_.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
    }
    void bytes(const std::uint8_t* data, std::size_t len) { out_.insert(out_.end(), data, data + len); }
    template <std::size_t N>
    void fixedBytes(const std::array<std::uint8_t, N>& arr) {
        bytes(arr.data(), arr.size());
    }
    void str(const std::string& s) {
        u32(static_cast<std::uint32_t>(s.size()));
        out_.insert(out_.end(), s.begin(), s.end());
    }
    void kdfParams(const crypto::KdfParams& p) {
        u32(p.memoryKiB);
        u32(p.iterations);
        u32(p.parallelism);
    }
    void keySlot(const KeySlot& slot) {
        kdfParams(slot.kdfParams);
        fixedBytes(slot.salt);
        fixedBytes(slot.nonce);
        fixedBytes(slot.wrappedDek);
    }
    std::vector<std::uint8_t> take() { return std::move(out_); }

private:
    std::vector<std::uint8_t> out_;
};

class Reader {
public:
    explicit Reader(const std::vector<std::uint8_t>& data) : data_(data) {}

    std::uint8_t u8() {
        require(1);
        return data_[pos_++];
    }
    std::uint16_t u16() {
        require(2);
        const std::uint16_t v =
            static_cast<std::uint16_t>(data_[pos_]) | static_cast<std::uint16_t>(data_[pos_ + 1] << 8);
        pos_ += 2;
        return v;
    }
    std::uint32_t u32() {
        require(4);
        const std::uint32_t v = static_cast<std::uint32_t>(data_[pos_]) |
                                 (static_cast<std::uint32_t>(data_[pos_ + 1]) << 8) |
                                 (static_cast<std::uint32_t>(data_[pos_ + 2]) << 16) |
                                 (static_cast<std::uint32_t>(data_[pos_ + 3]) << 24);
        pos_ += 4;
        return v;
    }
    template <std::size_t N>
    std::array<std::uint8_t, N> fixedBytes() {
        require(N);
        std::array<std::uint8_t, N> out{};
        std::copy(data_.begin() + static_cast<std::ptrdiff_t>(pos_),
                  data_.begin() + static_cast<std::ptrdiff_t>(pos_ + N), out.begin());
        pos_ += N;
        return out;
    }
    std::string str() {
        const auto len = u32();
        if (len > kMaxQuestionBytes) {
            throw VaultFileException(VaultFileError::Truncated, "Recovery question length exceeds sanity cap");
        }
        require(len);
        std::string out(data_.begin() + static_cast<std::ptrdiff_t>(pos_),
                        data_.begin() + static_cast<std::ptrdiff_t>(pos_ + len));
        pos_ += len;
        return out;
    }
    crypto::KdfParams kdfParams() {
        crypto::KdfParams p;
        p.memoryKiB = u32();
        p.iterations = u32();
        p.parallelism = u32();
        return p;
    }
    KeySlot keySlot() {
        KeySlot slot;
        slot.kdfParams = kdfParams();
        slot.salt = fixedBytes<crypto::kSaltBytes>();
        slot.nonce = fixedBytes<crypto::kNonceBytes>();
        slot.wrappedDek = fixedBytes<kWrappedDekBytes>();
        return slot;
    }
    std::size_t position() const { return pos_; }
    std::size_t remaining() const { return data_.size() - pos_; }

private:
    void require(std::size_t n) const {
        if (pos_ + n > data_.size()) {
            throw VaultFileException(VaultFileError::Truncated, "Vault file is truncated or corrupted");
        }
    }
    const std::vector<std::uint8_t>& data_;
    std::size_t pos_ = 0;
};

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

// Serializes everything EXCEPT the ciphertext — this exact byte sequence is
// what's passed as the body AEAD's associated data, binding every key slot
// and the recovery questions to the encrypted body.
std::vector<std::uint8_t> serializeHeader(const VaultFileHeader& header, std::uint32_t ciphertextLen) {
    Writer w;
    w.bytes(reinterpret_cast<const std::uint8_t*>(kMagic), kMagicSize);
    w.u16(header.version);
    w.u8(header.kdfAlgId);
    w.u8(header.aeadAlgId);
    w.keySlot(header.passwordSlot);
    w.u8(header.recoveryEnabled ? 1 : 0);
    if (header.recoveryEnabled) {
        w.u8(static_cast<std::uint8_t>(header.recoveryQuestions.size()));
        for (const auto& q : header.recoveryQuestions) {
            w.str(q);
        }
        w.keySlot(header.recoverySlot);
    }
    w.fixedBytes(header.bodyNonce);
    w.u32(ciphertextLen);
    return w.take();
}

[[noreturn]] void fail(VaultFileError code, const char* message) {
    throw VaultFileException(code, message);
}

void writeVaultFileInternal(const std::filesystem::path& path, const std::vector<std::uint8_t>& headerBytes,
                             const std::vector<std::uint8_t>& ciphertext) {
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

    if (bytes.size() < kMagicSize + 2 + kChecksumBytes) {
        fail(VaultFileError::Truncated, "Vault file is too small to be valid");
    }
    if (std::memcmp(bytes.data(), kMagic, kMagicSize) != 0) {
        fail(VaultFileError::BadMagic, "Not a lusakey vault file (bad magic bytes)");
    }

    const std::vector<std::uint8_t> storedChecksum(bytes.end() - static_cast<std::ptrdiff_t>(kChecksumBytes),
                                                     bytes.end());
    const std::vector<std::uint8_t> bodyAndHeaderBytes(bytes.begin(),
                                                         bytes.end() - static_cast<std::ptrdiff_t>(kChecksumBytes));

    RawVaultFile raw;
    Reader r(bodyAndHeaderBytes);
    r.fixedBytes<kMagicSize>(); // already validated above; just advance past it

    raw.header.version = r.u16();
    if (raw.header.version != kCurrentVersion) {
        fail(VaultFileError::UnsupportedVersion, "Unsupported vault file version");
    }
    raw.header.kdfAlgId = r.u8();
    raw.header.aeadAlgId = r.u8();
    raw.header.passwordSlot = r.keySlot();
    raw.header.recoveryEnabled = (r.u8() != 0);
    if (raw.header.recoveryEnabled) {
        const auto questionCount = r.u8();
        raw.header.recoveryQuestions.reserve(questionCount);
        for (std::uint8_t i = 0; i < questionCount; ++i) {
            raw.header.recoveryQuestions.push_back(r.str());
        }
        raw.header.recoverySlot = r.keySlot();
    }
    raw.header.bodyNonce = r.fixedBytes<crypto::kNonceBytes>();
    const auto ciphertextLen = r.u32();

    const auto headerLen = r.position();
    raw.headerBytes.assign(bodyAndHeaderBytes.begin(),
                            bodyAndHeaderBytes.begin() + static_cast<std::ptrdiff_t>(headerLen));

    if (r.remaining() != ciphertextLen) {
        fail(VaultFileError::Truncated, "Vault file size does not match its header");
    }
    raw.ciphertext.assign(bodyAndHeaderBytes.begin() + static_cast<std::ptrdiff_t>(headerLen),
                          bodyAndHeaderBytes.end());

    const auto computed = checksumOf(raw.headerBytes, raw.ciphertext);
    if (!std::equal(computed.begin(), computed.end(), storedChecksum.begin())) {
        fail(VaultFileError::ChecksumMismatch,
             "Vault file failed its integrity checksum (corrupted or truncated)");
    }

    return raw;
}

crypto::SecureBuffer unwrapDek(const KeySlot& slot, std::string_view secret) {
    crypto::SecureBuffer kek = crypto::deriveKey(secret, slot.salt.data(), slot.kdfParams);
    try {
        const std::vector<std::uint8_t> wrapped(slot.wrappedDek.begin(), slot.wrappedDek.end());
        auto plain = crypto::aeadDecrypt(kek, slot.nonce.data(), /*aad=*/{}, wrapped);
        crypto::SecureBuffer dek(plain.size());
        std::copy(plain.begin(), plain.end(), dek.data());
        std::fill(plain.begin(), plain.end(), std::uint8_t{0}); // best-effort scrub of the transient vector
        return dek;
    } catch (const std::exception&) {
        throw VaultFileException(VaultFileError::AuthenticationFailed,
                                  "Wrong password/answers, or this key slot has been tampered with");
    }
}

KeySlot makeKeySlot(const crypto::SecureBuffer& dek, std::string_view secret, const crypto::KdfParams& kdfParams) {
    KeySlot slot;
    slot.kdfParams = kdfParams;

    const auto saltBytes = crypto::randomBytes(crypto::kSaltBytes);
    std::copy(saltBytes.begin(), saltBytes.end(), slot.salt.begin());
    const auto nonceBytes = crypto::randomBytes(crypto::kNonceBytes);
    std::copy(nonceBytes.begin(), nonceBytes.end(), slot.nonce.begin());

    crypto::SecureBuffer kek = crypto::deriveKey(secret, slot.salt.data(), slot.kdfParams);
    const std::vector<std::uint8_t> dekBytes(dek.data(), dek.data() + dek.size());
    const auto wrapped = crypto::aeadEncrypt(kek, slot.nonce.data(), /*aad=*/{}, dekBytes);
    if (wrapped.size() != slot.wrappedDek.size()) {
        throw std::logic_error("makeKeySlot: unexpected wrapped DEK size");
    }
    std::copy(wrapped.begin(), wrapped.end(), slot.wrappedDek.begin());
    return slot;
}

std::vector<std::uint8_t> decryptBody(const RawVaultFile& raw, const crypto::SecureBuffer& dek) {
    try {
        return crypto::aeadDecrypt(dek, raw.header.bodyNonce.data(), raw.headerBytes, raw.ciphertext);
    } catch (const std::exception&) {
        fail(VaultFileError::AuthenticationFailed,
             "Vault body could not be decrypted (the DEK doesn't match, or the file is corrupted/tampered)");
    }
}

void writeVault(const std::filesystem::path& path,
                 const crypto::SecureBuffer& dek,
                 const KeySlot& passwordSlot,
                 bool recoveryEnabled,
                 const std::vector<std::string>& recoveryQuestions,
                 const KeySlot& recoverySlot,
                 const std::vector<std::uint8_t>& payload) {
    if (recoveryEnabled && recoveryQuestions.size() > kMaxRecoveryQuestions) {
        fail(VaultFileError::IoError, "Too many recovery questions");
    }

    VaultFileHeader header;
    header.version = kCurrentVersion;
    header.kdfAlgId = kKdfAlgArgon2id;
    header.aeadAlgId = kAeadAlgXChaCha20Poly1305;
    header.passwordSlot = passwordSlot;
    header.recoveryEnabled = recoveryEnabled;
    header.recoveryQuestions = recoveryEnabled ? recoveryQuestions : std::vector<std::string>{};
    header.recoverySlot = recoveryEnabled ? recoverySlot : KeySlot{};

    const auto bodyNonceBytes = crypto::randomBytes(crypto::kNonceBytes);
    std::copy(bodyNonceBytes.begin(), bodyNonceBytes.end(), header.bodyNonce.begin());

    const auto headerBytes = serializeHeader(header, static_cast<std::uint32_t>(payload.size() + crypto::kTagBytes));
    const auto ciphertext = crypto::aeadEncrypt(dek, header.bodyNonce.data(), headerBytes, payload);

    writeVaultFileInternal(path, headerBytes, ciphertext);
}

void createNew(const std::filesystem::path& path,
               std::string_view password,
               const crypto::KdfParams& kdfParams,
               const std::vector<std::uint8_t>& payload) {
    crypto::SecureBuffer dek(crypto::kDerivedKeyBytes);
    crypto::randomBytes(dek.data(), dek.size());

    const KeySlot passwordSlot = makeKeySlot(dek, password, kdfParams);
    writeVault(path, dek, passwordSlot, /*recoveryEnabled=*/false, {}, KeySlot{}, payload);
}

} // namespace lusakey::core::vault
