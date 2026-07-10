#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <vector>

#include "lusakey/core/crypto/kdf.h"
#include "lusakey/core/crypto/random.h"
#include "lusakey/core/vault/vault_file.h"

using namespace lusakey::core;
using namespace lusakey::core::vault;

namespace {

std::filesystem::path tempVaultPath(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

} // namespace

TEST_CASE("vault file round-trips a payload through createNew and readRaw+unwrapDek+decryptBody", "[vault_file]") {
    const auto path = tempVaultPath("lusakey_vf_roundtrip.lusakey");
    std::filesystem::remove(path);

    const std::vector<std::uint8_t> payload = {'p', 'a', 'y', 'l', 'o', 'a', 'd'};
    createNew(path, "master password", crypto::KdfParams::fast(), payload);

    const auto raw = readRaw(path);
    REQUIRE_FALSE(raw.header.recoveryEnabled);
    const auto dek = unwrapDek(raw.header.passwordSlot, "master password");
    const auto decrypted = decryptBody(raw, dek);
    REQUIRE(decrypted == payload);

    std::filesystem::remove(path);
}

TEST_CASE("vault file rejects the wrong password", "[vault_file]") {
    const auto path = tempVaultPath("lusakey_vf_wrongpw.lusakey");
    std::filesystem::remove(path);

    createNew(path, "correct password", crypto::KdfParams::fast(), {'x'});
    const auto raw = readRaw(path);

    bool threw = false;
    try {
        unwrapDek(raw.header.passwordSlot, "wrong password");
    } catch (const VaultFileException& e) {
        threw = true;
        REQUIRE(e.code() == VaultFileError::AuthenticationFailed);
    }
    REQUIRE(threw);

    std::filesystem::remove(path);
}

TEST_CASE("vault file supports a recovery slot alongside the password slot", "[vault_file]") {
    const auto path = tempVaultPath("lusakey_vf_recovery.lusakey");
    std::filesystem::remove(path);

    const std::vector<std::uint8_t> payload = {'r', 'e', 'c', 'o', 'v', 'e', 'r', 'y'};

    // Build a vault with both slots wrapping the SAME dek, as VaultService would.
    crypto::SecureBuffer dek(crypto::kDerivedKeyBytes);
    crypto::randomBytes(dek.data(), dek.size());
    const auto passwordSlot = makeKeySlot(dek, "master password", crypto::KdfParams::fast());
    const auto recoverySlot = makeKeySlot(dek, "fluffy|blue", crypto::KdfParams::fast());
    writeVault(path, dek, passwordSlot, /*recoveryEnabled=*/true, {"Pet's name?", "Favorite color?"}, recoverySlot,
               payload);

    const auto raw = readRaw(path);
    REQUIRE(raw.header.recoveryEnabled);
    REQUIRE(raw.header.recoveryQuestions.size() == 2);
    REQUIRE(raw.header.recoveryQuestions[0] == "Pet's name?");
    REQUIRE(raw.header.recoveryQuestions[1] == "Favorite color?");

    // Unlock via password.
    {
        const auto unlockedDek = unwrapDek(raw.header.passwordSlot, "master password");
        REQUIRE(decryptBody(raw, unlockedDek) == payload);
    }
    // Unlock via recovery answers.
    {
        const auto unlockedDek = unwrapDek(raw.header.recoverySlot, "fluffy|blue");
        REQUIRE(decryptBody(raw, unlockedDek) == payload);
    }
    // Wrong recovery answer fails.
    {
        REQUIRE_THROWS_AS(unwrapDek(raw.header.recoverySlot, "wrong|answer"), VaultFileException);
    }

    std::filesystem::remove(path);
}

TEST_CASE("vault file detects a corrupted body via checksum mismatch", "[vault_file]") {
    const auto path = tempVaultPath("lusakey_vf_corrupt.lusakey");
    std::filesystem::remove(path);

    createNew(path, "master password", crypto::KdfParams::fast(), {'x', 'y', 'z'});
    const auto sizeBefore = std::filesystem::file_size(path);

    {
        // Flip the last byte of the ciphertext (just before the trailing
        // 32-byte checksum) to simulate on-disk corruption.
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        REQUIRE(file.good());
        file.seekp(static_cast<std::streamoff>(sizeBefore) - 33);
        const char corruptByte = 0x7F;
        file.write(&corruptByte, 1);
    }

    bool threw = false;
    try {
        readRaw(path);
    } catch (const VaultFileException& e) {
        threw = true;
        REQUIRE(e.code() == VaultFileError::ChecksumMismatch);
    }
    REQUIRE(threw);

    std::filesystem::remove(path);
}

TEST_CASE("vault file rejects a file with bad magic bytes", "[vault_file]") {
    const auto path = tempVaultPath("lusakey_vf_badmagic.lusakey");
    std::filesystem::remove(path);

    {
        std::ofstream file(path, std::ios::binary);
        const std::vector<char> garbage(200, 'Z');
        file.write(garbage.data(), static_cast<std::streamsize>(garbage.size()));
    }

    bool threw = false;
    try {
        readRaw(path);
    } catch (const VaultFileException& e) {
        threw = true;
        REQUIRE(e.code() == VaultFileError::BadMagic);
    }
    REQUIRE(threw);

    std::filesystem::remove(path);
}
