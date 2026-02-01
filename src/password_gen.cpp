#include "password_gen.h"

#include <windows.h>
#include <bcrypt.h>
#include <vector>

namespace {
    wchar_t RandomChar(const std::wstring& pool) {
        ULONG idx = 0;
        BCryptGenRandom(nullptr, (PUCHAR)&idx, sizeof(idx), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        return pool[idx % pool.size()];
    }
}

namespace passgen {
    std::wstring Generate(int length, bool lower, bool upper, bool digits, bool symbols) {
        if (length <= 0) return L"";
        std::wstring pool;
        if (lower) pool += L"abcdefghijklmnopqrstuvwxyz";
        if (upper) pool += L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (digits) pool += L"0123456789";
        if (symbols) pool += L"!@#$%^&*()-_=+[]{};:,.<>/?";
        if (pool.empty()) pool = L"abcdefghijklmnopqrstuvwxyz";

        std::wstring out;
        out.reserve(length);
        for (int i = 0; i < length; ++i) {
            out.push_back(RandomChar(pool));
        }
        return out;
    }
}
