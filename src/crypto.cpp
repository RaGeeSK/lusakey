#include "crypto.h"

#include <windows.h>
#include <bcrypt.h>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace {
    const unsigned char kMagic[4] = { 'L', 'S', 'K', '1' };
    const ULONG kIterations = 120000;
    const ULONG kSaltLen = 16;
    const ULONG kNonceLen = 12;
    const ULONG kTagLen = 16;
    const ULONG kKeyLen = 32;

    bool RandomBytes(std::vector<unsigned char>& out, size_t len) {
        out.resize(len);
        return BCryptGenRandom(nullptr, out.data(), (ULONG)out.size(), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
    }

    bool DeriveKey(const std::wstring& password, const std::vector<unsigned char>& salt, std::vector<unsigned char>& key) {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
            return false;
        }
        key.resize(kKeyLen);
        NTSTATUS status = BCryptDeriveKeyPBKDF2(
            hAlg,
            (PUCHAR)password.c_str(),
            (ULONG)(password.size() * sizeof(wchar_t)),
            (PUCHAR)salt.data(),
            (ULONG)salt.size(),
            kIterations,
            key.data(),
            (ULONG)key.size(),
            0
        );
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return status == 0;
    }

    void WriteU32(std::vector<unsigned char>& out, ULONG v) {
        out.push_back((unsigned char)(v & 0xFF));
        out.push_back((unsigned char)((v >> 8) & 0xFF));
        out.push_back((unsigned char)((v >> 16) & 0xFF));
        out.push_back((unsigned char)((v >> 24) & 0xFF));
    }

    bool ReadU32(const std::vector<unsigned char>& in, size_t& off, ULONG& v) {
        if (off + 4 > in.size()) return false;
        v = (ULONG)in[off] |
            ((ULONG)in[off + 1] << 8) |
            ((ULONG)in[off + 2] << 16) |
            ((ULONG)in[off + 3] << 24);
        off += 4;
        return true;
    }
}

namespace crypto {
    void SecureZero(void* ptr, size_t len) {
        if (!ptr || len == 0) return;
        SecureZeroMemory(ptr, len);
    }

    bool Encrypt(const std::wstring& password, const std::vector<unsigned char>& plaintext, Blob& out) {
        std::vector<unsigned char> salt, nonce, key;
        if (!RandomBytes(salt, kSaltLen)) return false;
        if (!RandomBytes(nonce, kNonceLen)) return false;
        if (!DeriveKey(password, salt, key)) return false;

        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_KEY_HANDLE hKey = nullptr;
        if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0) != 0) return false;
        if (BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0) != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        ULONG objLen = 0;
        ULONG res = 0;
        if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objLen, sizeof(objLen), &res, 0) != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }
        std::vector<unsigned char> obj(objLen);
        if (BCryptGenerateSymmetricKey(hAlg, &hKey, obj.data(), objLen, key.data(), (ULONG)key.size(), 0) != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        std::vector<unsigned char> ciphertext(plaintext.size());
        std::vector<unsigned char> tag(kTagLen);
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
        BCRYPT_INIT_AUTH_MODE_INFO(info);
        info.pbNonce = nonce.data();
        info.cbNonce = (ULONG)nonce.size();
        info.pbTag = tag.data();
        info.cbTag = (ULONG)tag.size();

        ULONG outLen = 0;
        NTSTATUS status = BCryptEncrypt(
            hKey,
            (PUCHAR)plaintext.data(),
            (ULONG)plaintext.size(),
            &info,
            nullptr,
            0,
            ciphertext.data(),
            (ULONG)ciphertext.size(),
            &outLen,
            0
        );
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        SecureZero(key.data(), key.size());
        if (status != 0) return false;

        out.data.clear();
        out.data.insert(out.data.end(), kMagic, kMagic + 4);
        WriteU32(out.data, 1);
        WriteU32(out.data, kSaltLen);
        WriteU32(out.data, kNonceLen);
        WriteU32(out.data, kTagLen);
        WriteU32(out.data, (ULONG)ciphertext.size());
        out.data.insert(out.data.end(), salt.begin(), salt.end());
        out.data.insert(out.data.end(), nonce.begin(), nonce.end());
        out.data.insert(out.data.end(), tag.begin(), tag.end());
        out.data.insert(out.data.end(), ciphertext.begin(), ciphertext.end());
        return true;
    }

    bool Decrypt(const std::wstring& password, const std::vector<unsigned char>& blob, std::vector<unsigned char>& plaintext) {
        size_t off = 0;
        if (blob.size() < 4) return false;
        if (memcmp(blob.data(), kMagic, 4) != 0) return false;
        off += 4;
        ULONG version = 0, saltLen = 0, nonceLen = 0, tagLen = 0, ctLen = 0;
        if (!ReadU32(blob, off, version)) return false;
        if (!ReadU32(blob, off, saltLen)) return false;
        if (!ReadU32(blob, off, nonceLen)) return false;
        if (!ReadU32(blob, off, tagLen)) return false;
        if (!ReadU32(blob, off, ctLen)) return false;
        if (version != 1) return false;
        if (off + saltLen + nonceLen + tagLen + ctLen > blob.size()) return false;

        std::vector<unsigned char> salt(blob.begin() + off, blob.begin() + off + saltLen);
        off += saltLen;
        std::vector<unsigned char> nonce(blob.begin() + off, blob.begin() + off + nonceLen);
        off += nonceLen;
        std::vector<unsigned char> tag(blob.begin() + off, blob.begin() + off + tagLen);
        off += tagLen;
        std::vector<unsigned char> ciphertext(blob.begin() + off, blob.begin() + off + ctLen);

        std::vector<unsigned char> key;
        if (!DeriveKey(password, salt, key)) return false;

        BCRYPT_ALG_HANDLE hAlg = nullptr;
        BCRYPT_KEY_HANDLE hKey = nullptr;
        if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0) != 0) return false;
        if (BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0) != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        ULONG objLen = 0;
        ULONG res = 0;
        if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objLen, sizeof(objLen), &res, 0) != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }
        std::vector<unsigned char> obj(objLen);
        if (BCryptGenerateSymmetricKey(hAlg, &hKey, obj.data(), objLen, key.data(), (ULONG)key.size(), 0) != 0) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return false;
        }

        plaintext.resize(ciphertext.size());
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
        BCRYPT_INIT_AUTH_MODE_INFO(info);
        info.pbNonce = nonce.data();
        info.cbNonce = (ULONG)nonce.size();
        info.pbTag = tag.data();
        info.cbTag = (ULONG)tag.size();

        ULONG outLen = 0;
        NTSTATUS status = BCryptDecrypt(
            hKey,
            ciphertext.data(),
            (ULONG)ciphertext.size(),
            &info,
            nullptr,
            0,
            plaintext.data(),
            (ULONG)plaintext.size(),
            &outLen,
            0
        );
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        SecureZero(key.data(), key.size());
        return status == 0;
    }
}
