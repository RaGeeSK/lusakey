#pragma once

#include <string>
#include <vector>

struct Entry {
    std::wstring title;
    std::wstring category;
    std::wstring username;
    std::wstring password;
    std::wstring url;
    std::wstring notes;
};

struct Vault {
    std::vector<Entry> entries;
};

namespace vault {
    std::wstring VaultPath();
    bool Load(const std::wstring& password, Vault& out);
    bool Save(const std::wstring& password, const Vault& in);
}
