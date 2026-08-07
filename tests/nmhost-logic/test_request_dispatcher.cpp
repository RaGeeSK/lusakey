#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include <nlohmann/json.hpp>

#include "lusakey/core/crypto/kdf.h"
#include "lusakey/core/vault/vault_service.h"
#include "lusakey/nmhost/request_dispatcher.h"

using lusakey::core::crypto::KdfParams;
using lusakey::core::vault::EntryDraft;
using lusakey::core::vault::VaultService;
using lusakey::nmhost::RequestDispatcher;

namespace {

std::filesystem::path tempVaultPath(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

// Every test uses the fast KDF profile so the suite doesn't pay Argon2id's
// deliberately expensive default cost on every run — same convention as
// tests/core/test_vault_service.cpp.
KdfParams testKdf() {
    return KdfParams::fast();
}

} // namespace

TEST_CASE("RequestDispatcher ping succeeds without a vault", "[nmhost][dispatcher]") {
    VaultService service;
    RequestDispatcher dispatcher(service, "/nonexistent/path/does/not/matter");
    const auto response = nlohmann::json::parse(dispatcher.dispatch(R"({"action":"ping"})"));
    REQUIRE(response["ok"].get<bool>() == true);
    REQUIRE(response["result"].get<std::string>() == "pong");
}

TEST_CASE("RequestDispatcher unlock/listEntries/getEntry/currentTotpCode round-trip a real vault",
          "[nmhost][dispatcher]") {
    const auto path = tempVaultPath("lusakey_nmhost_dispatch.lusakey");
    std::filesystem::remove(path);

    EntryDraft draft;
    draft.title = "Example";
    draft.username = "alice";
    draft.password = "hunter2";
    draft.url = "https://example.com";
    lusakey::core::vault::EntryId entryId;
    {
        VaultService seed;
        seed.createVault(path, "master password", testKdf());
        entryId = seed.addEntry(draft);
    }

    // Fresh VaultService, exactly like a real nmhost process starting cold.
    VaultService service;
    RequestDispatcher dispatcher(service, path);

    {
        const auto response =
            nlohmann::json::parse(dispatcher.dispatch(R"({"action":"unlock","password":"master password"})"));
        REQUIRE(response["ok"].get<bool>() == true);
    }
    {
        const auto response = nlohmann::json::parse(dispatcher.dispatch(R"({"action":"listEntries"})"));
        REQUIRE(response["ok"].get<bool>() == true);
        REQUIRE(response["result"].size() == 1);
        REQUIRE(response["result"][0]["title"].get<std::string>() == "Example");
    }
    {
        const auto request = nlohmann::json{{"action", "getEntry"}, {"id", entryId}}.dump();
        const auto response = nlohmann::json::parse(dispatcher.dispatch(request));
        REQUIRE(response["ok"].get<bool>() == true);
        REQUIRE(response["result"]["username"].get<std::string>() == "alice");
        REQUIRE(response["result"]["password"].get<std::string>() == "hunter2");
    }
    {
        const auto request = nlohmann::json{{"action", "generatePassword"}, {"length", 12}}.dump();
        const auto response = nlohmann::json::parse(dispatcher.dispatch(request));
        REQUIRE(response["ok"].get<bool>() == true);
        REQUIRE(response["result"]["password"].get<std::string>().size() == 12);
    }
    {
        const auto response = nlohmann::json::parse(dispatcher.dispatch(R"({"action":"lock"})"));
        REQUIRE(response["ok"].get<bool>() == true);
        // Locked again -> listEntries must fail now.
        const auto after = nlohmann::json::parse(dispatcher.dispatch(R"({"action":"listEntries"})"));
        REQUIRE(after["ok"].get<bool>() == false);
        REQUIRE(after["code"].get<std::string>() == "NotUnlocked");
    }

    std::filesystem::remove(path);
}

TEST_CASE("RequestDispatcher unlock reports WrongPassword as a structured error", "[nmhost][dispatcher]") {
    const auto path = tempVaultPath("lusakey_nmhost_wrongpw.lusakey");
    std::filesystem::remove(path);
    {
        VaultService seed;
        seed.createVault(path, "correct password", testKdf());
    }

    VaultService service;
    RequestDispatcher dispatcher(service, path);
    const auto response =
        nlohmann::json::parse(dispatcher.dispatch(R"({"action":"unlock","password":"wrong password"})"));
    REQUIRE(response["ok"].get<bool>() == false);
    REQUIRE(response["code"].get<std::string>() == "WrongPassword");

    std::filesystem::remove(path);
}

TEST_CASE("RequestDispatcher getEntry reports EntryNotFound as a structured error", "[nmhost][dispatcher]") {
    const auto path = tempVaultPath("lusakey_nmhost_entrynotfound.lusakey");
    std::filesystem::remove(path);
    {
        VaultService seed;
        seed.createVault(path, "master password", testKdf());
    }

    VaultService service;
    RequestDispatcher dispatcher(service, path);
    dispatcher.dispatch(R"({"action":"unlock","password":"master password"})");

    const auto request = nlohmann::json{{"action", "getEntry"}, {"id", 999}}.dump();
    const auto response = nlohmann::json::parse(dispatcher.dispatch(request));
    REQUIRE(response["ok"].get<bool>() == false);
    REQUIRE(response["code"].get<std::string>() == "EntryNotFound");

    std::filesystem::remove(path);
}

TEST_CASE("RequestDispatcher rejects an unknown action", "[nmhost][dispatcher]") {
    VaultService service;
    RequestDispatcher dispatcher(service, "/nonexistent/path/does/not/matter");
    const auto response = nlohmann::json::parse(dispatcher.dispatch(R"({"action":"selfDestruct"})"));
    REQUIRE(response["ok"].get<bool>() == false);
    REQUIRE(response["code"].get<std::string>() == "UnknownAction");
}

TEST_CASE("RequestDispatcher does not throw on malformed JSON", "[nmhost][dispatcher]") {
    VaultService service;
    RequestDispatcher dispatcher(service, "/nonexistent/path/does/not/matter");
    const auto response = nlohmann::json::parse(dispatcher.dispatch("{not valid json"));
    REQUIRE(response["ok"].get<bool>() == false);
    REQUIRE(response["code"].get<std::string>() == "InvalidJson");
}

TEST_CASE("RequestDispatcher does not throw when a required param is missing", "[nmhost][dispatcher]") {
    VaultService service;
    RequestDispatcher dispatcher(service, "/nonexistent/path/does/not/matter");
    // "unlock" with no "password" field at all.
    const auto response = nlohmann::json::parse(dispatcher.dispatch(R"({"action":"unlock"})"));
    REQUIRE(response["ok"].get<bool>() == false);
    REQUIRE(response["code"].get<std::string>() == "InvalidRequest");
}
