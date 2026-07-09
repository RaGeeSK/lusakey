#pragma once

#include <string>

namespace lusakey::core::util {

struct PasswordGeneratorOptions {
    unsigned int length = 20;
    bool includeUppercase = true;
    bool includeLowercase = true;
    bool includeDigits = true;
    bool includeSymbols = true;
    bool excludeAmbiguous = false; // excludes visually similar characters: 0/O, 1/l/I, etc.
};

// Generates a random password via crypto::randomBytes (OS CSPRNG, with
// rejection sampling to avoid modulo bias) — never std::rand/std::mt19937.
// Guarantees at least one character from each enabled class.
//
// Throws std::invalid_argument if no character class is enabled, length is
// 0, or length is too short to fit one of each enabled class.
std::string generatePassword(const PasswordGeneratorOptions& options = {});

// Rough 0-4 strength score for a password-strength meter UI. A heuristic
// (length + character-class diversity), not a full entropy estimator —
// sufficient for a traffic-light indicator, not for security decisions.
int estimatePasswordStrength(const std::string& password);

} // namespace lusakey::core::util
