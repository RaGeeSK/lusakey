#pragma once

#include <string>

namespace lusakey::nmhost {

enum class BrowserLoginState { Pending, Approved, Denied, Expired, Failed };

struct BrowserLoginResult {
    BrowserLoginState state = BrowserLoginState::Pending;
    std::string password;
    std::string error;
};

// Starts a one-time browser login handoff and opens the GUI confirmation
// window. The GUI writes the approved password using the current user's
// Windows DPAPI key, so the extension never receives or stores it.
std::string beginBrowserLogin();

// Reads and consumes a one-time GUI response. An approved response contains
// the password only in process memory; its on-disk DPAPI blob is deleted.
BrowserLoginResult consumeBrowserLogin(const std::string& token);

} // namespace lusakey::nmhost
