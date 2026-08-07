#pragma once

#include <filesystem>

namespace lusakey::nmhost {

// Qt-free equivalent of AppController::defaultVaultPath() (app/bridge/
// app_controller.cpp) — nmhost has no QStandardPaths available, so this
// reconstructs the same on-disk location by hand, so a GUI-created vault
// and nmhost agree on where to find it without any path being passed
// between them.
//
// Honors LUSAKEY_TEST_VAULT_DIR exactly like the GUI does (see AGENTS.md) —
// set the same value for both processes to point them at one throwaway
// vault for manual cross-testing without touching a real profile.
std::filesystem::path defaultVaultPath();

} // namespace lusakey::nmhost
