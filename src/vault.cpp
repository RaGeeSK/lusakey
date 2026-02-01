#include "vault.h"
#include "crypto.h"

#include <windows.h>
#include <shlobj.h>
#include <fstream>

namespace {
    std::wstring Escape(const std::wstring& s) {
        std::wstring out;
        out.reserve(s.size());
        for (wchar_t c : s) {
            if (c == L'\\' || c == L'\t' || c == L'\n') out.push_back(L'\\');
            if (c == L'\t') out.push_back(L't');
            else if (c == L'\n') out.push_back(L'n');
            else out.push_back(c);
        }
        return out;
    }

    std::wstring Unescape(const std::wstring& s) {
        std::wstring out;
        out.reserve(s.size());
        bool esc = false;
        for (wchar_t c : s) {
            if (!esc && c == L'\\') {
                esc = true;
                continue;
            }
            if (esc) {
                if (c == L't') out.push_back(L'\t');
                else if (c == L'n') out.push_back(L'\n');
                else out.push_back(c);
                esc = false;
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

    std::vector<unsigned char> ToUtf8(const std::wstring& w) {
        if (w.empty()) return {};
        int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        std::vector<unsigned char> out(len);
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), (LPSTR)out.data(), len, nullptr, nullptr);
        return out;
    }

    std::wstring FromUtf8(const std::vector<unsigned char>& data) {
        if (data.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)data.data(), (int)data.size(), nullptr, 0);
        std::wstring out(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)data.data(), (int)data.size(), out.data(), len);
        return out;
    }

    std::vector<unsigned char> Serialize(const Vault& v) {
        std::wstring text;
        for (const auto& e : v.entries) {
            text += Escape(e.title) + L"\t" +
                Escape(e.category) + L"\t" +
                Escape(e.username) + L"\t" +
                Escape(e.password) + L"\t" +
                Escape(e.url) + L"\t" +
                Escape(e.notes) + L"\n";
        }
        return ToUtf8(text);
    }

    Vault Deserialize(const std::vector<unsigned char>& bytes) {
        Vault v;
        std::wstring text = FromUtf8(bytes);
        size_t start = 0;
        while (start < text.size()) {
            size_t end = text.find(L'\n', start);
            if (end == std::wstring::npos) end = text.size();
            std::wstring line = text.substr(start, end - start);
            if (!line.empty()) {
                Entry e;
                size_t p1 = line.find(L'\t');
                size_t p2 = line.find(L'\t', p1 == std::wstring::npos ? line.size() : p1 + 1);
                size_t p3 = line.find(L'\t', p2 == std::wstring::npos ? line.size() : p2 + 1);
                size_t p4 = line.find(L'\t', p3 == std::wstring::npos ? line.size() : p3 + 1);
                size_t p5 = line.find(L'\t', p4 == std::wstring::npos ? line.size() : p4 + 1);
                if (p1 != std::wstring::npos) e.title = Unescape(line.substr(0, p1));
                if (p2 != std::wstring::npos) e.category = Unescape(line.substr(p1 + 1, p2 - p1 - 1));
                if (p3 != std::wstring::npos) e.username = Unescape(line.substr(p2 + 1, p3 - p2 - 1));
                if (p4 != std::wstring::npos) e.password = Unescape(line.substr(p3 + 1, p4 - p3 - 1));
                if (p5 != std::wstring::npos) {
                    e.url = Unescape(line.substr(p4 + 1, p5 - p4 - 1));
                    e.notes = Unescape(line.substr(p5 + 1));
                } else if (p4 != std::wstring::npos) {
                    // Backward compatibility with older 5-field format
                    e.url = Unescape(line.substr(p3 + 1, p4 - p3 - 1));
                    e.notes = Unescape(line.substr(p4 + 1));
                }
                v.entries.push_back(e);
            }
            start = end + 1;
        }
        return v;
    }

    bool ReadFile(const std::wstring& path, std::vector<unsigned char>& out) {
        HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(h, &size)) {
            CloseHandle(h);
            return false;
        }
        if (size.QuadPart < 0) {
            CloseHandle(h);
            return false;
        }
        out.resize((size_t)size.QuadPart);
        DWORD read = 0;
        BOOL ok = TRUE;
        if (!out.empty()) {
            ok = ::ReadFile(h, out.data(), (DWORD)out.size(), &read, nullptr);
        }
        CloseHandle(h);
        return ok && read == out.size();
    }

    bool WriteFile(const std::wstring& path, const std::vector<unsigned char>& data) {
        HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;
        DWORD written = 0;
        BOOL ok = TRUE;
        if (!data.empty()) {
            ok = ::WriteFile(h, data.data(), (DWORD)data.size(), &written, nullptr);
        }
        CloseHandle(h);
        return ok && written == data.size();
    }
}

namespace vault {
    std::wstring VaultPath() {
        wchar_t folder[MAX_PATH];
        SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, folder);
        std::wstring dir = std::wstring(folder) + L"\\LusaKey";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir + L"\\vault.dat";
    }

    bool Load(const std::wstring& password, Vault& out) {
        std::vector<unsigned char> blob;
        if (!ReadFile(VaultPath(), blob)) return false;
        std::vector<unsigned char> plaintext;
        if (!crypto::Decrypt(password, blob, plaintext)) return false;
        out = Deserialize(plaintext);
        return true;
    }

    bool Save(const std::wstring& password, const Vault& in) {
        std::vector<unsigned char> plaintext = Serialize(in);
        crypto::Blob blob;
        if (!crypto::Encrypt(password, plaintext, blob)) return false;
        return WriteFile(VaultPath(), blob.data);
    }
}
