#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "lusakey/core/crypto/kdf.h"
#include "lusakey/core/crypto/secure_bytes.h"
#include "lusakey/core/util/password_generator.h"
#include "lusakey/core/vault/entry.h"
#include "lusakey/core/vault/vault_model.h"

namespace lusakey::core::vault {

enum class ServiceError {
    WrongPassword,
    FileCorrupt,
    FileNotFound,
    NotUnlocked,
    EntryNotFound,
    IoError,
    InvalidArgument,
};

class ServiceException : public std::runtime_error {
public:
    ServiceException(ServiceError code, const std::string& message)
        : std::runtime_error(message), code_(code) {}

    ServiceError code() const noexcept { return code_; }

private:
    ServiceError code_;
};

struct EntryFilter {
    std::string searchText;              // matched against title/username/url, case-insensitive
    std::optional<FolderId> folderId;
    std::optional<std::string> tag;
};

enum class ImportMode { Merge, Replace };

// The single entry point to a vault's data and lifecycle. Has NO Qt types in
// its public API by design: the GUI (via a thin C++/QML bridge in app/) and
// a future native-messaging-host (nmhost/) both consume this exact same
// class, so vault logic is implemented exactly once.
//
// Not thread-safe — callers serialize access. The GUI does so implicitly via
// Qt's single-threaded event loop; nmhost would do so via its own
// single-threaded stdin read loop. Move-only (a decrypted vault's state
// should never be silently duplicated).
class VaultService {
public:
    VaultService() = default;
    VaultService(const VaultService&) = delete;
    VaultService& operator=(const VaultService&) = delete;
    VaultService(VaultService&&) = default;
    VaultService& operator=(VaultService&&) = default;

    // Creates a brand-new vault at `path` and immediately persists it
    // (unlocked, empty). Throws ServiceException(InvalidArgument) if
    // `masterPassword` is empty.
    void createVault(const std::filesystem::path& path, std::string_view masterPassword,
                      const crypto::KdfParams& kdfParams = crypto::KdfParams::balanced());

    // Opens and decrypts `path` with `masterPassword`. Throws
    // ServiceException with FileNotFound / FileCorrupt / WrongPassword.
    void unlock(const std::filesystem::path& path, std::string_view masterPassword);

    // Zeroes the in-memory decrypted state and the derived key. Safe to call
    // when already locked.
    void lock();

    bool isUnlocked() const { return unlocked_; }

    // Re-keys the vault under a new master password (fresh salt, full
    // re-encrypt). Throws ServiceException(WrongPassword) if `oldPassword`
    // doesn't match, or (InvalidArgument) if `newPassword` is empty.
    void changeMasterPassword(std::string_view oldPassword, std::string_view newPassword);

    // All of the following require isUnlocked() — otherwise throw
    // ServiceException(NotUnlocked).
    std::vector<EntrySummary> listEntries(const EntryFilter& filter = {}) const;
    Entry getEntry(EntryId id) const; // throws ServiceException(EntryNotFound)
    EntryId addEntry(const EntryDraft& draft);
    void updateEntry(EntryId id, const EntryDraft& draft); // throws ServiceException(EntryNotFound)
    void removeEntry(EntryId id);                          // throws ServiceException(EntryNotFound)

    // Current TOTP code for entry `id`'s TotpSpec (zero-padded, e.g. "007123").
    // Throws ServiceException(EntryNotFound) or (InvalidArgument) if the
    // entry has no TOTP configured.
    std::string currentTotpCode(
        EntryId id, std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) const;
    unsigned int totpSecondsRemaining(
        EntryId id, std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) const;

    // Stateless utility; does not require the vault to be unlocked, but
    // grouped on VaultService to keep a single call surface for the GUI/host.
    std::string generatePassword(const util::PasswordGeneratorOptions& options = {}) const;

    // Re-encrypts (fresh nonce, same key/salt — the KDF does NOT re-run) and
    // writes the vault to its currently open path. Called automatically by
    // every mutating method above; exposed publicly in case a caller wants
    // to force a flush.
    void save();

    // Copies the currently open (already-encrypted) vault file verbatim to
    // `destPath` — the file is self-contained, so "export" needs no
    // re-encryption step.
    void exportVaultTo(const std::filesystem::path& destPath) const;

    // Opens `srcPath` with `srcPassword` and merges (default) or replaces
    // the current in-memory vault with its contents, then saves. On Merge,
    // imported entries are added as new entries (fresh ids, no folder
    // assignment) — de-duplication by title/username is a UI-level concern.
    void importVaultFrom(const std::filesystem::path& srcPath, std::string_view srcPassword, ImportMode mode);

    using ChangeCallback = std::function<void()>;
    // Registers a callback fired after every mutation (add/update/remove/
    // import). Plain std::function, not a Qt signal, so nmhost can use this
    // too. Returns a token to pass to removeChangeListener.
    int onChanged(ChangeCallback callback);
    void removeChangeListener(int token);

private:
    void requireUnlocked() const;
    void notifyChanged();

    bool unlocked_ = false;
    std::filesystem::path path_;
    crypto::SecureBuffer key_{0};
    std::array<std::uint8_t, crypto::kSaltBytes> salt_{};
    crypto::KdfParams kdfParams_;
    VaultModel model_;
    std::vector<std::pair<int, ChangeCallback>> listeners_;
    int nextListenerToken_ = 1;
};

} // namespace lusakey::core::vault
