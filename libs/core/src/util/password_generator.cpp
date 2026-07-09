#include "lusakey/core/util/password_generator.h"

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "lusakey/core/crypto/random.h"

namespace lusakey::core::util {

namespace {

constexpr char kUppercase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr char kUppercaseNoAmbiguous[] = "ABCDEFGHJKLMNPQRSTUVWXYZ"; // excludes I, O
constexpr char kLowercase[] = "abcdefghijklmnopqrstuvwxyz";
constexpr char kLowercaseNoAmbiguous[] = "abcdefghjkmnpqrstuvwxyz"; // excludes i, l, o
constexpr char kDigits[] = "0123456789";
constexpr char kDigitsNoAmbiguous[] = "23456789"; // excludes 0, 1
constexpr char kSymbols[] = "!@#$%^&*()-_=+[]{};:,.<>?";

// Uniform selection from `alphabet` via rejection sampling on a random byte —
// avoids the modulo bias a naive `randomByte() % alphabet.size()` introduces.
char pickUniform(const std::string& alphabet) {
    const auto size = alphabet.size();
    const unsigned int limit = 256u - (256u % static_cast<unsigned int>(size));
    unsigned int value;
    do {
        std::uint8_t byte = 0;
        crypto::randomBytes(&byte, 1);
        value = byte;
    } while (value >= limit);
    return alphabet[value % size];
}

// Fisher-Yates shuffle using the same rejection-sampling CSPRNG approach, so
// the characters guaranteed-per-class aren't always at the front of the result.
void shuffle(std::string& s) {
    for (std::size_t i = s.size(); i-- > 1;) {
        const unsigned int span = static_cast<unsigned int>(i) + 1;
        const unsigned int limit = 256u - (256u % span);
        unsigned int value;
        do {
            std::uint8_t byte = 0;
            crypto::randomBytes(&byte, 1);
            value = byte;
        } while (value >= limit);
        std::swap(s[i], s[value % span]);
    }
}

} // namespace

std::string generatePassword(const PasswordGeneratorOptions& options) {
    std::string alphabet;
    std::vector<std::string> requiredClasses;

    const auto appendClass = [&](bool enabled, const char* full, const char* noAmbiguous) {
        if (!enabled) return;
        const std::string cls = options.excludeAmbiguous ? noAmbiguous : full;
        alphabet += cls;
        requiredClasses.push_back(cls);
    };
    appendClass(options.includeUppercase, kUppercase, kUppercaseNoAmbiguous);
    appendClass(options.includeLowercase, kLowercase, kLowercaseNoAmbiguous);
    appendClass(options.includeDigits, kDigits, kDigitsNoAmbiguous);
    appendClass(options.includeSymbols, kSymbols, kSymbols); // symbols has no "ambiguous" variant

    if (alphabet.empty()) {
        throw std::invalid_argument("generatePassword: at least one character class must be enabled");
    }
    if (options.length == 0) {
        throw std::invalid_argument("generatePassword: length must be greater than 0");
    }
    if (options.length < requiredClasses.size()) {
        throw std::invalid_argument(
            "generatePassword: length too short to include one character of each enabled class");
    }

    std::string result;
    result.reserve(options.length);
    for (const auto& cls : requiredClasses) {
        result.push_back(pickUniform(cls));
    }
    while (result.size() < options.length) {
        result.push_back(pickUniform(alphabet));
    }

    shuffle(result);
    return result;
}

int estimatePasswordStrength(const std::string& password) {
    if (password.empty()) {
        return 0;
    }

    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;
    bool hasSymbol = false;
    for (const unsigned char c : password) {
        if (c >= 'A' && c <= 'Z') {
            hasUpper = true;
        } else if (c >= 'a' && c <= 'z') {
            hasLower = true;
        } else if (c >= '0' && c <= '9') {
            hasDigit = true;
        } else {
            hasSymbol = true;
        }
    }
    const int classes =
        static_cast<int>(hasUpper) + static_cast<int>(hasLower) + static_cast<int>(hasDigit) + static_cast<int>(hasSymbol);

    const auto length = password.size();
    int score = 0;
    if (length >= 8) score++;
    if (length >= 12) score++;
    if (length >= 16) score++;
    if (classes >= 3) score++;
    if (classes >= 4 && length >= 12) score++;

    return score > 4 ? 4 : score;
}

} // namespace lusakey::core::util
