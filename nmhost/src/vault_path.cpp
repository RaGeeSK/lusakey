#include "lusakey/nmhost/vault_path.h"

#include <cstdlib>

namespace lusakey::nmhost {

namespace {

std::filesystem::path resolveOsDataDir() {
#ifdef _WIN32
    // QStandardPaths::AppDataLocation for an app with OrganizationName ==
    // ApplicationName == "lusakey" resolves to %APPDATA%\lusakey\lusakey —
    // confirmed empirically against a real build of the GUI app (not just
    // assumed from Qt's documented algorithm): see AGENTS.md.
    const char* appData = std::getenv("APPDATA");
    if (appData && *appData) {
        return std::filesystem::path(appData) / "lusakey" / "lusakey";
    }
    return std::filesystem::path(".");
#elif defined(__APPLE__)
    // Unverified — this repo has never been built/run on macOS (see
    // AGENTS.md). Matches Qt's documented AppDataLocation algorithm, not
    // empirically confirmed like the Windows branch above.
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::filesystem::path(home) / "Library" / "Application Support" / "lusakey";
    }
    return std::filesystem::path(".");
#else
    // Unverified — this repo has never been built/run on Linux (see
    // AGENTS.md). Matches the XDG base directory spec Qt's
    // AppDataLocation follows there, not empirically confirmed.
    const char* xdgDataHome = std::getenv("XDG_DATA_HOME");
    if (xdgDataHome && *xdgDataHome) {
        return std::filesystem::path(xdgDataHome) / "lusakey";
    }
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::filesystem::path(home) / ".local" / "share" / "lusakey";
    }
    return std::filesystem::path(".");
#endif
}

} // namespace

std::filesystem::path defaultVaultPath() {
    if (const char* testDir = std::getenv("LUSAKEY_TEST_VAULT_DIR"); testDir && *testDir) {
        const std::filesystem::path dir(testDir);
        std::filesystem::create_directories(dir);
        return dir / "vault.lusakey";
    }

    const auto dir = resolveOsDataDir();
    std::filesystem::create_directories(dir);
    return dir / "vault.lusakey";
}

} // namespace lusakey::nmhost
