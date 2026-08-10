#include "lusakey/nmhost/browser_login.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "lusakey/nmhost/vault_path.h"

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <wincrypt.h>
#endif

namespace lusakey::nmhost {

namespace {

std::filesystem::path handoffDirectory() {
    const auto dir = defaultVaultPath().parent_path() / "browser-login";
    std::filesystem::create_directories(dir);
    return dir;
}

std::filesystem::path requestPath(const std::string& token) {
    return handoffDirectory() / (token + ".request");
}

std::filesystem::path responsePath(const std::string& token) {
    return handoffDirectory() / (token + ".response");
}

std::filesystem::path deniedPath(const std::string& token) {
    return handoffDirectory() / (token + ".denied");
}

bool looksLikeToken(const std::string& token) {
    return token.size() == 32 && token.find_first_not_of("0123456789abcdef") == std::string::npos;
}

#ifdef _WIN32
std::string makeToken() {
    std::array<unsigned char, 16> bytes{};
    if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        throw std::runtime_error("could not create a secure browser-login token");
    }
    constexpr char hex[] = "0123456789abcdef";
    std::string token;
    token.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        token.push_back(hex[byte >> 4U]);
        token.push_back(hex[byte & 0x0fU]);
    }
    return token;
}

std::filesystem::path currentExecutablePath() {
    std::array<wchar_t, 32768> buffer{};
    const auto count = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (count == 0 || count >= buffer.size()) {
        throw std::runtime_error("could not determine native-host path");
    }
    return std::filesystem::path(buffer.data());
}

void openApprovalWindow(const std::string& token) {
    const auto hostPath = currentExecutablePath();
    const auto hostDir = hostPath.parent_path();
    const std::array candidates{
        hostDir.parent_path() / "app" / "lusakey.exe", // CMake build layout
        hostDir / "lusakey.exe",                         // packaged layout
    };

    const auto it = std::find_if(candidates.begin(), candidates.end(), [](const auto& path) {
        return std::filesystem::exists(path);
    });
    if (it == candidates.end()) {
        throw std::runtime_error("lusakey.exe was not found next to the native host");
    }

#ifdef LUSAKEY_QT_BIN_DIR
    // A packaged app carries Qt DLLs next to lusakey.exe. In an un-packaged
    // CMake build they remain in Qt's bin directory, which Chrome does not
    // put on PATH when it starts the native host. Pass that directory on to
    // the approval-window process so the same build is testable directly.
    const std::wstring qtBin = LUSAKEY_QT_BIN_DIR;
    const DWORD size = GetEnvironmentVariableW(L"PATH", nullptr, 0);
    std::wstring path(size, L'\0');
    if (size > 0) {
        GetEnvironmentVariableW(L"PATH", path.data(), size);
        if (!path.empty() && path.back() == L'\0') {
            path.pop_back();
        }
    }
    SetEnvironmentVariableW(L"PATH", (qtBin + L";" + path).c_str());
#endif

    const auto args = std::wstring(L"--browser-login-token ") + std::wstring(token.begin(), token.end());
    const auto launched = ShellExecuteW(nullptr, L"open", it->c_str(), args.c_str(), nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<std::intptr_t>(launched) <= 32) {
        throw std::runtime_error("could not open lusakey approval window");
    }
}
#endif

} // namespace

std::string beginBrowserLogin() {
#ifdef _WIN32
    const auto token = makeToken();
    {
        std::ofstream request(requestPath(token), std::ios::binary | std::ios::trunc);
        if (!request) {
            throw std::runtime_error("could not create browser-login request");
        }
        request << "pending";
    }

    try {
        openApprovalWindow(token);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(requestPath(token), ignored);
        throw;
    }
    return token;
#else
    throw std::runtime_error("browser approval is currently supported only on Windows");
#endif
}

BrowserLoginResult consumeBrowserLogin(const std::string& token) {
    if (!looksLikeToken(token)) {
        return {BrowserLoginState::Failed, {}, "invalid browser-login token"};
    }
    if (std::filesystem::exists(deniedPath(token))) {
        std::error_code ignored;
        std::filesystem::remove(deniedPath(token), ignored);
        std::filesystem::remove(requestPath(token), ignored);
        return {BrowserLoginState::Denied, {}, {}};
    }
    if (!std::filesystem::exists(responsePath(token))) {
        return std::filesystem::exists(requestPath(token))
            ? BrowserLoginResult{BrowserLoginState::Pending, {}, {}}
            : BrowserLoginResult{BrowserLoginState::Expired, {}, "browser-login request was not found"};
    }

#ifdef _WIN32
    std::ifstream input(responsePath(token), std::ios::binary);
    std::vector<unsigned char> encrypted((std::istreambuf_iterator<char>(input)), {});
    if (encrypted.empty()) {
        return {BrowserLoginState::Failed, {}, "empty browser-login response"};
    }

    DATA_BLOB source{static_cast<DWORD>(encrypted.size()), encrypted.data()};
    DATA_BLOB plain{};
    if (!CryptUnprotectData(&source, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &plain)) {
        return {BrowserLoginState::Failed, {}, "could not decrypt browser-login approval"};
    }
    std::string password(reinterpret_cast<const char*>(plain.pbData), plain.cbData);
    SecureZeroMemory(plain.pbData, plain.cbData);
    LocalFree(plain.pbData);

    std::error_code ignored;
    std::filesystem::remove(responsePath(token), ignored);
    std::filesystem::remove(requestPath(token), ignored);
    return {BrowserLoginState::Approved, std::move(password), {}};
#else
    return {BrowserLoginState::Failed, {}, "browser approval is currently supported only on Windows"};
#endif
}

} // namespace lusakey::nmhost
