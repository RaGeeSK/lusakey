#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "lusakey/core/totp/hotp.h"
#include "lusakey/core/totp/otpauth_uri.h"
#include "lusakey/core/totp/totp.h"

using namespace lusakey::core::totp;

namespace {

std::vector<std::uint8_t> asciiBytes(const std::string& s) {
    return std::vector<std::uint8_t>(s.begin(), s.end());
}

std::chrono::system_clock::time_point atUnixSeconds(std::int64_t seconds) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
}

} // namespace

TEST_CASE("HOTP matches RFC 4226 Appendix D test vectors", "[totp][rfc4226]") {
    const auto secret = asciiBytes("12345678901234567890");
    const std::string expected[10] = {
        "755224", "287082", "359152", "969429", "338314", "254676", "287922", "162583", "399871", "520489",
    };
    for (std::uint64_t counter = 0; counter < 10; ++counter) {
        const auto code = hotp(secret, counter, 6, HashAlgorithm::SHA1);
        REQUIRE(formatCode(code, 6) == expected[counter]);
    }
}

TEST_CASE("TOTP matches RFC 6238 Appendix B test vectors", "[totp][rfc6238]") {
    const auto secretSha1 = asciiBytes("12345678901234567890");
    const auto secretSha256 = asciiBytes("12345678901234567890123456789012");
    const auto secretSha512 = asciiBytes("1234567890123456789012345678901234567890123456789012345678901234");

    const TotpParams sha1Params{HashAlgorithm::SHA1, 8, 30};
    const TotpParams sha256Params{HashAlgorithm::SHA256, 8, 30};
    const TotpParams sha512Params{HashAlgorithm::SHA512, 8, 30};

    struct Vector {
        std::int64_t time;
        const char* sha1;
        const char* sha256;
        const char* sha512;
    };
    const Vector vectors[] = {
        {59, "94287082", "46119246", "90693936"},
        {1111111109, "07081804", "68084774", "25091201"},
        {1111111111, "14050471", "67062674", "99943326"},
        {1234567890, "89005924", "91819424", "93441116"},
        {2000000000, "69279037", "90698825", "38618901"},
        {20000000000, "65353130", "77737706", "47863826"},
    };

    for (const auto& v : vectors) {
        const auto now = atUnixSeconds(v.time);
        REQUIRE(formatCode(totp(secretSha1, sha1Params, now), 8) == v.sha1);
        REQUIRE(formatCode(totp(secretSha256, sha256Params, now), 8) == v.sha256);
        REQUIRE(formatCode(totp(secretSha512, sha512Params, now), 8) == v.sha512);
    }
}

TEST_CASE("parseOtpAuthUri extracts label/issuer/params and decodes the secret", "[totp][otpauth]") {
    const std::string uri =
        "otpauth://totp/Example:alice@example.com?secret=JBSWY3DPEHPK3PXP&issuer=Example"
        "&algorithm=SHA1&digits=6&period=30";
    const auto parsed = parseOtpAuthUri(uri);

    REQUIRE(parsed.issuer == "Example");
    REQUIRE(parsed.label == "Example:alice@example.com");
    REQUIRE(parsed.params.algorithm == HashAlgorithm::SHA1);
    REQUIRE(parsed.params.digits == 6);
    REQUIRE(parsed.params.periodSeconds == 30);
    REQUIRE_FALSE(parsed.secret.empty());
}

TEST_CASE("OtpAuthUri round-trips through toOtpAuthUri/parseOtpAuthUri", "[totp][otpauth]") {
    const std::string uri =
        "otpauth://totp/Example:alice@example.com?secret=JBSWY3DPEHPK3PXP&issuer=Example"
        "&algorithm=SHA256&digits=8&period=60";
    const auto parsed = parseOtpAuthUri(uri);

    const auto reserialized = toOtpAuthUri(parsed);
    const auto reparsed = parseOtpAuthUri(reserialized);

    REQUIRE(reparsed.secret == parsed.secret);
    REQUIRE(reparsed.issuer == parsed.issuer);
    REQUIRE(reparsed.params.algorithm == parsed.params.algorithm);
    REQUIRE(reparsed.params.digits == parsed.params.digits);
    REQUIRE(reparsed.params.periodSeconds == parsed.params.periodSeconds);
}

TEST_CASE("parseOtpAuthUri rejects a non-otpauth URI", "[totp][otpauth]") {
    REQUIRE_THROWS_AS(parseOtpAuthUri("https://example.com"), std::invalid_argument);
}

TEST_CASE("parseOtpAuthUri rejects a URI missing the secret parameter", "[totp][otpauth]") {
    REQUIRE_THROWS_AS(parseOtpAuthUri("otpauth://totp/Example:alice@example.com?issuer=Example"),
                       std::invalid_argument);
}
