#pragma once

#include <string>

namespace passgen {
    std::wstring Generate(int length, bool lower, bool upper, bool digits, bool symbols);
}
