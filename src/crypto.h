#pragma once

#include <string>
#include <vector>

namespace crypto {
    struct Blob {
        std::vector<unsigned char> data;
    };

    bool Encrypt(const std::wstring& password, const std::vector<unsigned char>& plaintext, Blob& out);
    bool Decrypt(const std::wstring& password, const std::vector<unsigned char>& blob, std::vector<unsigned char>& plaintext);
    void SecureZero(void* ptr, size_t len);
}
