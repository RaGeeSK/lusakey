#include <catch2/catch_test_macros.hpp>

#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "lusakey/core/crypto/kdf.h"
#include "lusakey/core/totp/otpauth_uri.h"
#include "lusakey/core/vault/vault_service.h"

using namespace lusakey::core;
using namespace lusakey::core::vault;

namespace {

std::filesystem::path tempVaultPath(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

// Every test uses the fast KDF profile so the test suite doesn't pay
// Argon2id's deliberately expensive default cost on every run.
crypto::KdfParams testKdf() {
    return crypto::KdfParams::fast();
}

} // namespace

TEST_CASE("VaultService create -> add -> lock -> unlock round-trips an entry", "[vault_service]") {
    const auto path = tempVaultPath("lusakey_svc_roundtrip.lusakey");
    std::filesystem::remove(path);

    EntryId id;
    {
        VaultService service;
        service.createVault(path, "master password", testKdf());

        EntryDraft draft;
        draft.title = "Example";
        draft.username = "alice";
        draft.password = "hunter2";
        draft.url = "https://example.com";
        draft.tags = {"work"};
        id = service.addEntry(draft);

        service.lock();
        REQUIRE_FALSE(service.isUnlocked());
    }

    {
        VaultService service;
        service.unlock(path, "master password");
        REQUIRE(service.isUnlocked());

        const auto entry = service.getEntry(id);
        REQUIRE(entry.title == "Example");
        REQUIRE(entry.username == "alice");
        REQUIRE(entry.password == "hunter2");
        REQUIRE(entry.tags == std::vector<std::string>{"work"});
    }

    std::filesystem::remove(path);
}

TEST_CASE("VaultService rejects the wrong password on unlock", "[vault_service]") {
    const auto path = tempVaultPath("lusakey_svc_wrongpw.lusakey");
    std::filesystem::remove(path);

    {
        VaultService service;
        service.createVault(path, "correct password", testKdf());
    }

    VaultService service;
    bool threw = false;
    try {
        service.unlock(path, "wrong password");
    } catch (const ServiceException& e) {
        threw = true;
        REQUIRE(e.code() == ServiceError::WrongPassword);
    }
    REQUIRE(threw);
    REQUIRE_FALSE(service.isUnlocked());

    std::filesystem::remove(path);
}

TEST_CASE("VaultService methods require the vault to be unlocked", "[vault_service]") {
    VaultService service;
    REQUIRE_THROWS_AS(service.listEntries(), ServiceException);
    REQUIRE_THROWS_AS(service.addEntry(EntryDraft{}), ServiceException);
}

TEST_CASE("VaultService folder methods require the vault to be unlocked", "[vault_service][folders]") {
    VaultService service;
    REQUIRE_THROWS_AS(service.listFolders(), ServiceException);
    REQUIRE_THROWS_AS(service.addFolder("Work"), ServiceException);
    REQUIRE_THROWS_AS(service.renameFolder(1, "Work2"), ServiceException);
    REQUIRE_THROWS_AS(service.removeFolder(1), ServiceException);
}

TEST_CASE("VaultService folder CRUD add/rename/remove round-trips and orphans entries", "[vault_service][folders]") {
    const auto path = tempVaultPath("lusakey_svc_folder_crud.lusakey");
    std::filesystem::remove(path);

    VaultService service;
    service.createVault(path, "master password", testKdf());

    const auto folderId = service.addFolder("Work");
    REQUIRE(service.listFolders().size() == 1);
    REQUIRE(service.listFolders()[0].name == "Work");

    EntryDraft draft;
    draft.title = "Work Entry";
    draft.folderId = folderId;
    const auto entryId = service.addEntry(draft);
    REQUIRE(service.getEntry(entryId).folderId == folderId);

    service.renameFolder(folderId, "Work Stuff");
    REQUIRE(service.listFolders()[0].name == "Work Stuff");

    service.removeFolder(folderId);
    REQUIRE(service.listFolders().empty());
    REQUIRE_FALSE(service.getEntry(entryId).folderId.has_value()); // orphaned, not deleted

    std::filesystem::remove(path);
}

TEST_CASE("VaultService addFolder/renameFolder reject empty names", "[vault_service][folders]") {
    const auto path = tempVaultPath("lusakey_svc_folder_empty_name.lusakey");
    std::filesystem::remove(path);

    VaultService service;
    service.createVault(path, "master password", testKdf());

    REQUIRE_THROWS_AS(service.addFolder(""), ServiceException);
    const auto folderId = service.addFolder("Real Folder");
    REQUIRE_THROWS_AS(service.renameFolder(folderId, ""), ServiceException);

    std::filesystem::remove(path);
}

TEST_CASE("VaultService renameFolder/removeFolder throw FolderNotFound for a missing id", "[vault_service][folders]") {
    const auto path = tempVaultPath("lusakey_svc_folder_missing.lusakey");
    std::filesystem::remove(path);

    VaultService service;
    service.createVault(path, "master password", testKdf());

    bool threwRename = false;
    try {
        service.renameFolder(999, "New Name");
    } catch (const ServiceException& e) {
        threwRename = true;
        REQUIRE(e.code() == ServiceError::FolderNotFound);
    }
    REQUIRE(threwRename);

    bool threwRemove = false;
    try {
        service.removeFolder(999);
    } catch (const ServiceException& e) {
        threwRemove = true;
        REQUIRE(e.code() == ServiceError::FolderNotFound);
    }
    REQUIRE(threwRemove);

    std::filesystem::remove(path);
}

TEST_CASE("VaultService listEntries filters by folder including the unfiled sentinel", "[vault_service][folders]") {
    const auto path = tempVaultPath("lusakey_svc_folder_filter.lusakey");
    std::filesystem::remove(path);

    VaultService service;
    service.createVault(path, "master password", testKdf());

    const auto folderA = service.addFolder("A");
    const auto folderB = service.addFolder("B");

    EntryDraft inA;
    inA.title = "In A";
    inA.folderId = folderA;
    service.addEntry(inA);

    EntryDraft inB;
    inB.title = "In B";
    inB.folderId = folderB;
    service.addEntry(inB);

    EntryDraft unfiled;
    unfiled.title = "Unfiled";
    service.addEntry(unfiled);

    REQUIRE(service.listEntries().size() == 3); // no filter -> all

    EntryFilter filterA;
    filterA.folderId = folderA;
    const auto resultsA = service.listEntries(filterA);
    REQUIRE(resultsA.size() == 1);
    REQUIRE(resultsA[0].title == "In A");

    EntryFilter filterUnfiled;
    filterUnfiled.folderId = FolderId{0};
    const auto resultsUnfiled = service.listEntries(filterUnfiled);
    REQUIRE(resultsUnfiled.size() == 1);
    REQUIRE(resultsUnfiled[0].title == "Unfiled");

    std::filesystem::remove(path);
}

TEST_CASE("VaultService folders persist through lock/unlock", "[vault_service][folders]") {
    const auto path = tempVaultPath("lusakey_svc_folder_persist.lusakey");
    std::filesystem::remove(path);

    FolderId folderId;
    {
        VaultService service;
        service.createVault(path, "master password", testKdf());
        folderId = service.addFolder("Persisted");
        EntryDraft draft;
        draft.title = "Persisted Entry";
        draft.folderId = folderId;
        service.addEntry(draft);
        service.lock();
    }

    VaultService service;
    service.unlock(path, "master password");
    const auto folders = service.listFolders();
    REQUIRE(folders.size() == 1);
    REQUIRE(folders[0].id == folderId);
    REQUIRE(folders[0].name == "Persisted");

    EntryFilter filter;
    filter.folderId = folderId;
    const auto entries = service.listEntries(filter);
    REQUIRE(entries.size() == 1);
    REQUIRE(entries[0].title == "Persisted Entry");

    std::filesystem::remove(path);
}

TEST_CASE("VaultService search filters by title/username/url", "[vault_service]") {
    const auto path = tempVaultPath("lusakey_svc_search.lusakey");
    std::filesystem::remove(path);

    VaultService service;
    service.createVault(path, "master password", testKdf());

    EntryDraft a;
    a.title = "GitHub";
    a.username = "octocat";
    a.url = "https://github.com";
    service.addEntry(a);

    EntryDraft b;
    b.title = "Example Bank";
    b.username = "alice";
    b.url = "https://bank.example.com";
    service.addEntry(b);

    EntryFilter filter;
    filter.searchText = "github";
    const auto results = service.listEntries(filter);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].title == "GitHub");

    EntryFilter caseInsensitive;
    caseInsensitive.searchText = "ALICE";
    REQUIRE(service.listEntries(caseInsensitive).size() == 1);

    std::filesystem::remove(path);
}

TEST_CASE("VaultService computes a TOTP code for an entry with otpauth secret", "[vault_service]") {
    const auto path = tempVaultPath("lusakey_svc_totp.lusakey");
    std::filesystem::remove(path);

    VaultService service;
    service.createVault(path, "master password", testKdf());

    const auto parsed = totp::parseOtpAuthUri(
        "otpauth://totp/Example:alice@example.com?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"
        "&issuer=Example&algorithm=SHA1&digits=6&period=30");

    EntryDraft draft;
    draft.title = "Example";
    TotpSpec spec;
    spec.secret = parsed.secret;
    spec.params = parsed.params;
    draft.totp = spec;
    const auto id = service.addEntry(draft);

    const auto code = service.currentTotpCode(id);
    REQUIRE(code.size() == 6);
    for (const char c : code) {
        REQUIRE(std::isdigit(static_cast<unsigned char>(c)));
    }

    const auto remaining = service.totpSecondsRemaining(id);
    REQUIRE(remaining >= 1);
    REQUIRE(remaining <= 30);

    std::filesystem::remove(path);
}

TEST_CASE("VaultService generatePassword honors requested length", "[vault_service]") {
    VaultService service;
    const auto password = service.generatePassword({});
    REQUIRE(password.size() == 20); // default length
}

TEST_CASE("VaultService export/import merges entries from another vault", "[vault_service]") {
    const auto pathA = tempVaultPath("lusakey_svc_import_a.lusakey");
    const auto pathB = tempVaultPath("lusakey_svc_import_b.lusakey");
    std::filesystem::remove(pathA);
    std::filesystem::remove(pathB);

    {
        VaultService serviceA;
        serviceA.createVault(pathA, "password a", testKdf());
        EntryDraft draft;
        draft.title = "From Vault A";
        serviceA.addEntry(draft);
    }

    VaultService serviceB;
    serviceB.createVault(pathB, "password b", testKdf());
    EntryDraft existing;
    existing.title = "Already in B";
    serviceB.addEntry(existing);

    serviceB.importVaultFrom(pathA, "password a", ImportMode::Merge);

    const auto entries = serviceB.listEntries();
    REQUIRE(entries.size() == 2);

    bool foundA = false;
    bool foundB = false;
    for (const auto& e : entries) {
        if (e.title == "From Vault A") foundA = true;
        if (e.title == "Already in B") foundB = true;
    }
    REQUIRE(foundA);
    REQUIRE(foundB);

    std::filesystem::remove(pathA);
    std::filesystem::remove(pathB);
}

TEST_CASE("VaultService exportVaultTo copies an openable vault file", "[vault_service]") {
    const auto path = tempVaultPath("lusakey_svc_export_src.lusakey");
    const auto exportPath = tempVaultPath("lusakey_svc_export_dst.lusakey");
    std::filesystem::remove(path);
    std::filesystem::remove(exportPath);

    VaultService service;
    service.createVault(path, "master password", testKdf());
    EntryDraft draft;
    draft.title = "Exported Entry";
    service.addEntry(draft);
    service.exportVaultTo(exportPath);

    VaultService reopened;
    reopened.unlock(exportPath, "master password");
    const auto entries = reopened.listEntries();
    REQUIRE(entries.size() == 1);
    REQUIRE(entries[0].title == "Exported Entry");

    std::filesystem::remove(path);
    std::filesystem::remove(exportPath);
}

TEST_CASE("VaultService changeMasterPassword rotates the password slot without losing data", "[vault_service]") {
    const auto path = tempVaultPath("lusakey_svc_changepw.lusakey");
    std::filesystem::remove(path);

    VaultService service;
    service.createVault(path, "old password", testKdf());
    EntryDraft draft;
    draft.title = "Survives Rotation";
    service.addEntry(draft);

    REQUIRE_THROWS_AS(service.changeMasterPassword("wrong old password", "new password"), ServiceException);

    service.changeMasterPassword("old password", "new password");
    service.lock();

    {
        VaultService oldPasswordAttempt;
        REQUIRE_THROWS_AS(oldPasswordAttempt.unlock(path, "old password"), ServiceException);
    }

    VaultService reopened;
    reopened.unlock(path, "new password");
    const auto entries = reopened.listEntries();
    REQUIRE(entries.size() == 1);
    REQUIRE(entries[0].title == "Survives Rotation");

    std::filesystem::remove(path);
}

TEST_CASE("VaultService setupRecovery lets a vault be unlocked via secret-question answers", "[vault_service]") {
    const auto path = tempVaultPath("lusakey_svc_recovery.lusakey");
    std::filesystem::remove(path);

    REQUIRE_FALSE(VaultService::hasRecovery(path)); // file doesn't exist yet -> false, not a throw

    {
        VaultService service;
        service.createVault(path, "master password", testKdf());
        REQUIRE_FALSE(service.recoveryEnabled());

        EntryDraft draft;
        draft.title = "Recoverable Entry";
        service.addEntry(draft);

        service.setupRecovery({"Pet's name?", "Favorite color?"}, {" Fluffy ", "BLUE"}, testKdf());
        REQUIRE(service.recoveryEnabled());
        REQUIRE(service.recoveryQuestions().size() == 2);

        service.lock();
    }

    REQUIRE(VaultService::hasRecovery(path));
    const auto questions = VaultService::getRecoveryQuestions(path);
    REQUIRE(questions.size() == 2);
    REQUIRE(questions[0] == "Pet's name?");
    REQUIRE(questions[1] == "Favorite color?");

    // Wrong answers fail.
    {
        VaultService service;
        REQUIRE_THROWS_AS(service.unlockWithRecoveryAnswers(path, {"wrong", "wrong"}), ServiceException);
    }

    // Correct answers succeed, case/whitespace-insensitively, and recover the same data.
    {
        VaultService service;
        service.unlockWithRecoveryAnswers(path, {"fluffy", "blue"}); // different case/whitespace than when set up
        const auto entries = service.listEntries();
        REQUIRE(entries.size() == 1);
        REQUIRE(entries[0].title == "Recoverable Entry");

        // The password slot must still work too — recovery doesn't replace it.
        service.lock();
        service.unlock(path, "master password");
        REQUIRE(service.listEntries().size() == 1);
    }

    std::filesystem::remove(path);
}

TEST_CASE("VaultService unlockWithRecoveryAnswers throws RecoveryNotConfigured when none is set up", "[vault_service]") {
    const auto path = tempVaultPath("lusakey_svc_norecovery.lusakey");
    std::filesystem::remove(path);

    {
        VaultService service;
        service.createVault(path, "master password", testKdf());
    }

    VaultService service;
    bool threw = false;
    try {
        service.unlockWithRecoveryAnswers(path, {"anything"});
    } catch (const ServiceException& e) {
        threw = true;
        REQUIRE(e.code() == ServiceError::RecoveryNotConfigured);
    }
    REQUIRE(threw);

    std::filesystem::remove(path);
}

TEST_CASE("VaultService::resetVault deletes the vault file so a new one can be created", "[vault_service]") {
    const auto path = tempVaultPath("lusakey_svc_reset.lusakey");
    std::filesystem::remove(path);

    {
        VaultService service;
        service.createVault(path, "master password", testKdf());
    }
    REQUIRE(std::filesystem::exists(path));

    VaultService::resetVault(path);
    REQUIRE_FALSE(std::filesystem::exists(path));

    // A brand-new vault can be created at the same path afterward.
    VaultService service;
    service.createVault(path, "a different password", testKdf());
    REQUIRE(service.isUnlocked());

    std::filesystem::remove(path);
}
