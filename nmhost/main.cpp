#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <iostream>

#include "lusakey/core/vault/vault_service.h"
#include "lusakey/ipc/native_message_channel.h"
#include "lusakey/nmhost/request_dispatcher.h"
#include "lusakey/nmhost/vault_path.h"

int main() {
#ifdef _WIN32
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