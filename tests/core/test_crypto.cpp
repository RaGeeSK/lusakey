#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstring>
#include <vector>

#include "lusakey/core/crypto/aead.h"
#include "lusakey/core/crypto/kdf.h"
#include "lusakey/core/crypto/random.h"
#include "lusakey/core/crypto/secure_bytes.h"

using namespace lusakey::core::crypto;

namespace {

std::array<std::uint8_t, kSaltBytes> fixedSalt() {
    std::array<std::uint8_t, kSaltBytes> salt{};
    for (std::size_t i = 0; i < salt.size(); ++i) {
        salt[i] = static_cast<std::uint8_t>(i);
    }
    return salt;
}

std::array<std::uint8_t, kNonceBytes> fixedNonce() {
    std::array<std::uint8_t, kNonceBytes> nonce{};
    for (std::size_t i = 0; i < nonce.size(); ++i) {
        nonce[i] = static_cast<std::uint8_t>(0xA0 + i);
    }
    return nonce;
}

} // namespace

TEST_CASE("KDF is deterministic for the same password/salt/params", "[kdf]") {
    const auto salt = fixedSalt();
    const auto params = KdfParams::fast(); // fast profile keeps the test quick
    SecureBuffer key1 = deriveKey("correct horse battery staple", salt.data(), params);
    SecureBuffer key2 = deriveKey("correct horse battery staple", salt.data(), params);

    REQUIRE(key1.size() == key2.size());
    REQUIRE(std::memcmp(key1.data(), key2.data(), key1.size()) == 0);
}

TEST_CASE("KDF produces different keys for different salts", "[kdf]") {
    auto salt1 = fixedSalt();
    auto salt2 = fixedSalt();
    salt2[0] ^= 0xFF;
    const auto params = KdfParams::fast();

    SecureBuffer key1 = deriveKey("same password", salt1.data(), params);
    SecureBuffer key2 = deriveKey("same password", salt2.data(), params);

    REQUIRE(std::memcmp(key1.data(), key2.data(), key1.size()) != 0);
}

TEST_CASE("AEAD round-trip returns the original plaintext", "[aead]") {
    const auto salt = fixedSalt();
    const auto nonce = fixedNonce();
    SecureBuffer key = deriveKey("vault master password", salt.data(), KdfParams::fast());

    const std::vector<std::uint8_t> aad = {'h', 'd', 'r'};
    const std::vector<std::uint8_t> plaintext = {'s', 'e', 'c', 'r', 'e', 't', '!'};

    const auto ciphertext = aeadEncrypt(key, nonce.data(), aad, plaintext);
    const auto decrypted = aeadDecrypt(key, nonce.data(), aad, ciphertext);

    REQUIRE(decrypted == plaintext);
}

TEST_CASE("AEAD decrypt fails if the ciphertext is tampered with", "[aead]") {
    const auto salt = fixedSalt();
    const auto nonce = fixedNonce();
    SecureBuffer key = deriveKey("vault master password", salt.data(), KdfParams::fast());

    const std::vector<std::uint8_t> aad = {'h', 'd', 'r'};
    const std::vector<std::uint8_t> plaintext = {'s', 'e', 'c', 'r', 'e', 't', '!'};

    auto ciphertext = aeadEncrypt(key, nonce.data(), aad, plaintext);
    ciphertext[0] ^= 0x01;

    REQUIRE_THROWS_AS(aeadDecrypt(key, nonce.data(), aad, ciphertext), std::runtime_error);
}

TEST_CASE("AEAD decrypt fails if the associated data is tampered with", "[aead]") {
    const auto salt = fixedSalt();
    const auto nonce = fixedNonce();
    SecureBuffer key = deriveKey("vault master password", salt.data(), KdfParams::fast());

    const std::vector<std::uint8_t> aad = {'h', 'd', 'r'};
    const std::vector<std::uint8_t> tamperedAad = {'h', 'd', 'x'};
    const std::vector<std::uint8_t> plaintext = {'s', 'e', 'c', 'r', 'e', 't', '!'};

    const auto ciphertext = aeadEncrypt(key, nonce.data(), aad, plaintext);

    REQUIRE_THROWS_AS(aeadDecrypt(key, nonce.data(), tamperedAad, ciphertext), std::runtime_error);
}

TEST_CASE("AEAD decrypt fails with the wrong key", "[aead]") {
    const auto salt = fixedSalt();
    const auto nonce = fixedNonce();
    SecureBuffer key = deriveKey("vault master password", salt.data(), KdfParams::fast());
    SecureBuffer wrongKey = deriveKey("not the vault master password", salt.data(), KdfParams::fast());

    const std::vector<std::uint8_t> aad = {'h', 'd', 'r'};
    const std::vector<std::uint8_t> plaintext = {'s', 'e', 'c', 'r', 'e', 't', '!'};

    const auto ciphertext = aeadEncrypt(key, nonce.data(), aad, plaintext);

    REQUIRE_THROWS_AS(aeadDecrypt(wrongKey, nonce.data(), aad, ciphertext), std::runtime_error);
}

TEST_CASE("randomBytes produces non-trivial, non-repeating output", "[random]") {
    const auto a = randomBytes(32);
    const auto b = randomBytes(32);
    REQUIRE(a.size() == 32);
    REQUIRE(b.size() == 32);
    REQUIRE(a != b); // astronomically unlikely to collide if the CSPRNG works
}
