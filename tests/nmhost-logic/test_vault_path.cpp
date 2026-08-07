#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <string>

#include "lusakey/nmhost/vault_path.h"

namespace {

// Sets an environment variable for the duration of the enclosing scope,
// then restores whatever was there before (or unsets it, if it wasn't set)
// — so this test can't leak LUSAKEY_TEST_VAULT_DIR into any test that runs
// after it in the same process.
class ScopedEnvVar {
public:
    ScopedEnvVar(const char* name, const std::string& value) : name_(name) {
        if (const char* existing = std::getenv(name)) {
            hadPrevious_ = true;
            previous_ = existing;
        }
#ifdef _WIN32
        _putenv_s(name, value.c_str());
#else
        setenv(name, value.c_str(), 1);
#endif
    }

    ~ScopedEnvVar() {
#ifdef _WIN32
        _putenv_s(name_.c_str(), hadPrevious_ ? previous_.c_str() : "");
#else
        if (hadPrevious_) {
            setenv(name_.c_str(), previous_.c_str(), 1);
        } else {
            unsetenv(name_.c_str());
        }
#endif
    }

private:
    std::string name_;
    bool hadPrevious_ = false;
    std::string previous_;
};

} // namespace

TEST_CASE("nmhost defaultVaultPath honors LUSAKEY_TEST_VAULT_DIR", "[nmhost][vault_path]") {
    const auto testDir = std::filesystem::temp_directory_path() / "lusakey_nmhost_vault_path_test";
    ScopedEnvVar env("LUSAKEY_TEST_VAULT_DIR", testDir.string());

    const auto path = lusakey::nmhost::defaultVaultPath();
    REQUIRE(path.parent_path() == testDir);
    REQUIRE(path.filename() == "vault.lusakey");
    REQUIRE(std::filesystem::exists(testDir)); // create_directories side effect

    std::filesystem::remove_all(testDir);
}
