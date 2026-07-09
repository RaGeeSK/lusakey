#include "lusakey/core/totp/otpauth_uri.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "base32.h"

namespace lusakey::core::totp {

namespace {

int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string percentDecode(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '%' && i + 2 < input.size()) {
            const int hi = hexDigit(input[i + 1]);
            const int lo = hexDigit(input[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(input[i] == '+' ? ' ' : input[i]);
    }
    return out;
}

std::string percentEncode(std::string_view input) {
    static const char kHex[] = "0123456789ABCDEF";
    std::string out;
    for (const unsigned char c : input) {
        const bool isUnreserved =
            (std::isalnum(c) != 0) || c == '-' || c == '_' || c == '.' || c == '~';
        if (isUnreserved) {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(kHex[(c >> 4) & 0xF]);
            out.push_back(kHex[c & 0xF]);
        }
    }
    return out;
}

HashAlgorithm parseAlgorithm(const std::string& value) {
    std::string upper = value;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                    [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    if (upper == "SHA1") return HashAlgorithm::SHA1;
    if (upper == "SHA256") return HashAlgorithm::SHA256;
    if (upper == "SHA512") return HashAlgorithm::SHA512;
    throw std::invalid_argument("parseOtpAuthUri: unsupported algorithm '" + value + "'");
}

const char* algorithmName(HashAlgorithm algorithm) {
    switch (algorithm) {
        case HashAlgorithm::SHA1: return "SHA1";
        case HashAlgorithm::SHA256: return "SHA256";
        case HashAlgorithm::SHA512: return "SHA512";
    }
    return "SHA1";
}

} // namespace

OtpAuthUri parseOtpAuthUri(std::string_view uri) {
    constexpr std::string_view kPrefix = "otpauth://totp/";
    if (uri.substr(0, kPrefix.size()) != kPrefix) {
        throw std::invalid_argument("parseOtpAuthUri: expected an otpauth://totp/ URI");
    }

    const auto rest = uri.substr(kPrefix.size());
    const auto queryPos = rest.find('?');
    const auto labelPart = rest.substr(0, queryPos);
    const auto queryPart =
        (queryPos == std::string_view::npos) ? std::string_view{} : rest.substr(queryPos + 1);

    OtpAuthUri result;
    result.label = percentDecode(labelPart);

    // Default issuer: the "Issuer:Account" label-prefix convention, if present.
    // An explicit `issuer` query parameter (handled below) overrides this.
    if (const auto colonPos = result.label.find(':'); colonPos != std::string::npos) {
        result.issuer = result.label.substr(0, colonPos);
    }

    std::unordered_map<std::string, std::string> query;
    std::size_t pos = 0;
    while (pos < queryPart.size()) {
        const auto ampPos = queryPart.find('&', pos);
        const auto pairLen = (ampPos == std::string_view::npos) ? std::string_view::npos : ampPos - pos;
        const auto pair = queryPart.substr(pos, pairLen);
        if (const auto eqPos = pair.find('='); eqPos != std::string_view::npos) {
            query[percentDecode(pair.substr(0, eqPos))] = percentDecode(pair.substr(eqPos + 1));
        }
        if (ampPos == std::string_view::npos) break;
        pos = ampPos + 1;
    }

    const auto secretIt = query.find("secret");
    if (secretIt == query.end() || secretIt->second.empty()) {
        throw std::invalid_argument("parseOtpAuthUri: missing required 'secret' parameter");
    }
    result.secret = internal::base32Decode(secretIt->second);
    if (result.secret.empty()) {
        throw std::invalid_argument("parseOtpAuthUri: 'secret' parameter is not valid Base32");
    }

    if (const auto it = query.find("issuer"); it != query.end() && !it->second.empty()) {
        result.issuer = it->second;
    }

    result.params.algorithm = HashAlgorithm::SHA1;
    if (const auto it = query.find("algorithm"); it != query.end()) {
        result.params.algorithm = parseAlgorithm(it->second);
    }

    result.params.digits = 6;
    if (const auto it = query.find("digits"); it != query.end()) {
        result.params.digits = static_cast<unsigned int>(std::stoul(it->second));
    }

    result.params.periodSeconds = 30;
    if (const auto it = query.find("period"); it != query.end()) {
        result.params.periodSeconds = static_cast<unsigned int>(std::stoul(it->second));
    }

    return result;
}

std::string toOtpAuthUri(const OtpAuthUri& entry) {
    std::ostringstream oss;
    oss << "otpauth://totp/" << percentEncode(entry.label) << "?secret=" << internal::base32Encode(entry.secret)
        << "&issuer=" << percentEncode(entry.issuer) << "&algorithm=" << algorithmName(entry.params.algorithm)
        << "&digits=" << entry.params.digits << "&period=" << entry.params.periodSeconds;
    return oss.str();
}

} // namespace lusakey::core::totp
