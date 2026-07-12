#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <iostream>

#include "lusakey/core/vault/vault_service.h"
#include "lusakey/ipc/native_message_channel.h"
#include "lusakey/nmhost/request_dispatcher.h"
#include "lusakey/nmhost/vault_path.h"

// Native-messaging host: reads framed JSON requests from stdin, dispatches
// them against a single VaultService instance shared across the whole
// process lifetime, and writes framed JSON responses to stdout — the
// protocol a Chrome/Firefox extension speaks to a native host it launched.
// See RequestDispatcher for the actual action set/JSON shapes. Not
// installed or registered with any browser yet — see
// nmhost/native-messaging/README.md for how to exercise this manually.
int main() {
#ifdef _WIN32
    // Native messaging is raw binary framing (4-byte length prefix); on
    // Windows, stdin/stdout default to text mode, which would corrupt it by
    // translating \n <-> \r\n. Force binary mode before reading anything.
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    using lusakey::core::ipc::readMessage;
    using lusakey::core::ipc::writeMessage;
    using lusakey::core::vault::VaultService;
    using lusakey::nmhost::RequestDispatcher;

    VaultService vaultService;
    RequestDispatcher dispatcher(vaultService, lusakey::nmhost::defaultVaultPath());

    while (const auto message = readMessage(std::cin)) {
        writeMessage(std::cout, dispatcher.dispatch(*message));
    }

    return 0;
}
