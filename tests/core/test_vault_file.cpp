#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <vector>

#include "lusakey/core/crypto/kdf.h"
#include "lusakey/core/vault/vault_file.h"

using namespace lusakey::core;

namespace {

std::filesystem::path tempVaultPath(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

} // namespace

TEST_CASE("vault file round-trips a payload through write and open", "[vault_file]") {
    const auto path = tempVaultPath("lusakey_test_roundtrip.lusakey");
    std::filesystem::remove(path);

    const std::vector<std::uint8_t> payload = {'p', 'a', 'y', 'l', 'o', 'a', 'd'};
    vault::writeNew(path, "master password", crypto::KdfParams::fast(), payload);

    const auto decrypted = vault::openAndDecrypt(path, "master password");
    REQUIRE(decrypted == payload);

    std::filesystem::remove(path);
}

TEST_CASE("vault file rejects the wrong password", "[vault_file]") {
    const auto path = tempVaultPath("lusakey_test_wrongpw.lusakey");
    std::filesystem::remove(path);

    const std::vector<std::uint8_t> payload = {'x'};
    vault::writeNew(path, "correct password", crypto::KdfParams::fast(), payload);

    bool threw = false;
    try {
        vault::openAndDecrypt(path, "wrong password");
    } catch (const vault::VaultFileException& e) {
        threw = true;
        REQUIRE(e.code() == vault::VaultFileError::AuthenticationFailed);
    }
    REQUIRE(threw);

    std::filesystem::remove(path);
}

TEST_CASE("vault file detects a corrupted ciphertext via checksum mismatch", "[vault_file]") {
    const auto path = tempVaultPath("lusakey_test_corrupt.lusakey");
    std::filesystem::remove(path);

    const std::vector<std::uint8_t> payload = {'x', 'y', 'z'};
    vault::writeNew(path, "master password", crypto::KdfParams::fast(), payload);

    {
        // Byte offset 70 falls inside the ciphertext region (which starts at
        // the 68-byte header boundary) for this payload size — flip it to
        // simulate on-disk corruption without touching the trailing checksum.
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        REQUIRE(file.good());
        file.seekp(70);
        const char corruptByte = 0x7F;
        file.write(&corruptByte, 1);
    }

    bool threw = false;
    try {
        vault::openAndDecrypt(path, "master password");
    } catch (const vault::VaultFileException& e) {
        threw = true;
        REQUIRE(e.code() == vault::VaultFileError::ChecksumMismatch);
    }
    REQUIRE(threw);

    std::filesystem::remove(path);
}

TEST_CASE("vault file rejects a file with bad magic bytes", "[vault_file]") {
    const auto path = tempVaultPath("lusakey_test_badmagic.lusakey");
    std::filesystem::remove(path);

    {
        std::ofstream file(path, std::ios::binary);
        const std::vector<char> garbage(200, 'Z');
        file.write(garbage.data(), static_cast<std::streamsize>(garbage.size()));
    }

    bool threw = false;
    try {
        vault::readRaw(path);
    } catch (const vault::VaultFileException& e) {
        threw = true;
        REQUIRE(e.code() == vault::VaultFileError::BadMagic);
    }
    REQUIRE(threw);

    std::filesystem::remove(path);
}
