#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include "lusakey/core/vault/vault_service.h"

namespace lusakey::nmhost {

// Parses one native-messaging JSON request, dispatches it against a live
// VaultService, and returns the serialized JSON response as a string. Never
// throws — every failure (malformed JSON, missing/wrong-typed params, a
// thrown ServiceException, even generatePassword()'s std::invalid_argument)
// becomes a structured {"ok": false, "error": "...", "code": "..."} response
// instead. This is a machine API consumed by a browser extension, not a
// GUI — AppController's occasional "swallow the error, return empty" style
// is not appropriate here.
//
// Protocol: request {"action": "<name>", ...params}, response either
// {"ok": true, "result": ...} or {"ok": false, "error": "...", "code": "..."}.
// Actions: ping, requestAppLogin, getAppLoginStatus, lock, listEntries(searchText?),
// getEntry(id), getCredentialsForUrl(url), currentTotpCode(id),
// generatePassword(length?,
// includeUppercase?, includeLowercase?, includeDigits?, includeSymbols?,
// excludeAmbiguous?). Deliberately does NOT expose vault/folder creation —
// nmhost only ever opens a vault the GUI already created.
class RequestDispatcher {
public:
    RequestDispatcher(lusakey::core::vault::VaultService& service, std::filesystem::path vaultPath);

    std::string dispatch(const std::string& rawJson);

private:
    lusakey::core::vault::VaultService& service_;
    std::filesystem::path vaultPath_;
    std::unordered_map<std::string, std::string> browserLoginStates_;
};

} // namespace lusakey::nmhost
