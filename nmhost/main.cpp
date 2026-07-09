#include <iostream>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <nlohmann/json.hpp>

#include "lusakey/core/vault/vault_service.h"
#include "lusakey/ipc/native_message_channel.h"

// Stub native-messaging host. Its only job right now is to prove that
// VaultService (Qt-free, in libs/core) and the native-messaging framing
// (libs/ipc) link and run together outside the GUI process — the seam a
// future browser extension will attach to (see AGENTS.md, M9). There is no
// real request router yet, no manifest registration with any browser, and
// this binary is not installed or shipped anywhere; it only builds when
// explicitly requested via -DLUSAKEY_BUILD_NMHOST=ON.
int main() {
#ifdef _WIN32
    // Native messaging is a raw binary framing (4-byte length prefix); on
    // Windows, stdin/stdout default to text mode, which would corrupt it by
    // translating \n <-> \r\n. Force binary mode before reading anything.
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    using lusakey::core::ipc::readMessage;
    using lusakey::core::ipc::writeMessage;
    using lusakey::core::vault::VaultService;

    // Constructed to prove the type is linkable/usable from a process with
    // no QApplication/event loop; a real router would unlock a vault path
    // supplied by the extension and dispatch CRUD/TOTP requests against it.
    VaultService vaultService;
    (void)vaultService;

    while (const auto message = readMessage(std::cin)) {
        nlohmann::json request;
        try {
            request = nlohmann::json::parse(*message);
        } catch (const nlohmann::json::parse_error&) {
            writeMessage(std::cout, nlohmann::json{{"ok", false}, {"error", "invalid JSON"}}.dump());
            continue;
        }

        const std::string action = request.value("action", "");
        nlohmann::json response;
        if (action == "ping") {
            response = {{"ok", true}, {"result", "pong"}};
        } else {
            response = {{"ok", false}, {"error", "unknown action: " + action}};
        }
        writeMessage(std::cout, response.dump());
    }

    return 0;
}
