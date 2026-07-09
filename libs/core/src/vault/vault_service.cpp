#include "lusakey/core/vault/vault_service.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <system_error>
#include <utility>

#include "lusakey/core/crypto/random.h"
#include "lusakey/core/totp/totp.h"
#include "lusakey/core/vault/vault_file.h"

namespace lusakey::core::vault {

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

} // namespace

void VaultService::requireUnlocked() const {
    if (!unlocked_) {
        throw ServiceException(ServiceError::NotUnlocked, "Vault is locked");
    }
}

void VaultService::notifyChanged() {
    for (const auto& [token, callback] : listeners_) {
        if (callback) {
            callback();
        }
    }
}

void VaultService::createVault(const std::filesystem::path& path, std::string_view masterPassword,
                                const crypto::KdfParams& kdfParams) {
    if (masterPassword.empty()) {
        throw ServiceException(ServiceError::InvalidArgument, "Master password must not be empty");
    }

    path_ = path;
    kdfParams_ = kdfParams;
    model_ = VaultModel{};

    const auto saltBytes = crypto::randomBytes(crypto::kSaltBytes);
    std::copy(saltBytes.begin(), saltBytes.end(), salt_.begin());
    key_ = crypto::deriveKey(masterPassword, salt_.data(), kdfParams_);

    unlocked_ = true;
    save(); // persists an empty, valid, openable vault immediately
}

void VaultService::unlock(const std::filesystem::path& path, std::string_view masterPassword) {
    RawVaultFile raw;
    try {
        raw = readRaw(path);
    } catch (const VaultFileException& e) {
        if (e.code() == VaultFileError::NotFound) {
            throw ServiceException(ServiceError::FileNotFound, e.what());
        }
        throw ServiceException(ServiceError::FileCorrupt, e.what());
    }

    crypto::SecureBuffer candidateKey = crypto::deriveKey(masterPassword, raw.header.salt.data(), raw.header.kdfParams);

    std::vector<std::uint8_t> payload;
    try {
        payload = decryptPayload(raw, candidateKey);
    } catch (const VaultFileException&) {
        throw ServiceException(ServiceError::WrongPassword, "Incorrect master password");
    }

    VaultModel model;
    try {
        model = VaultModel::deserialize(payload);
    } catch (const std::exception& e) {
        throw ServiceException(ServiceError::FileCorrupt, e.what());
    }

    model_ = std::move(model);
    path_ = path;
    salt_ = raw.header.salt;
    kdfParams_ = raw.header.kdfParams;
    key_ = std::move(candidateKey);
    unlocked_ = true;
}

void VaultService::lock() {
    model_ = VaultModel{};
    key_.zero();
    key_ = crypto::SecureBuffer(0);
    unlocked_ = false;
}

void VaultService::changeMasterPassword(std::string_view oldPassword, std::string_view newPassword) {
    requireUnlocked();

    crypto::SecureBuffer check = crypto::deriveKey(oldPassword, salt_.data(), kdfParams_);
    // This check isn't the vault's security boundary (the AEAD tag on the
    // file is) — the vault is already unlocked in memory. It just guards
    // against a UI mistake by confirming the caller knows the current
    // password before rotating it.
    if (check.size() != key_.size() || std::memcmp(check.data(), key_.data(), check.size()) != 0) {
        throw ServiceException(ServiceError::WrongPassword, "Current password is incorrect");
    }
    if (newPassword.empty()) {
        throw ServiceException(ServiceError::InvalidArgument, "New password must not be empty");
    }

    const auto newSalt = crypto::randomBytes(crypto::kSaltBytes);
    std::copy(newSalt.begin(), newSalt.end(), salt_.begin());
    key_ = crypto::deriveKey(newPassword, salt_.data(), kdfParams_);
    save();
}

std::vector<EntrySummary> VaultService::listEntries(const EntryFilter& filter) const {
    requireUnlocked();

    const auto needle = toLower(filter.searchText);
    std::vector<EntrySummary> results;
    for (const auto& [id, entry] : model_.entries()) {
        if (filter.folderId && entry.folderId != filter.folderId) {
            continue;
        }
        if (filter.tag &&
            std::find(entry.tags.begin(), entry.tags.end(), *filter.tag) == entry.tags.end()) {
            continue;
        }
        if (!needle.empty() && toLower(entry.title).find(needle) == std::string::npos &&
            toLower(entry.username).find(needle) == std::string::npos &&
            toLower(entry.url).find(needle) == std::string::npos) {
            continue;
        }
        results.push_back(EntrySummary{entry.id, entry.title, entry.username, entry.totp.has_value()});
    }
    std::sort(results.begin(), results.end(),
              [](const EntrySummary& a, const EntrySummary& b) { return a.title < b.title; });
    return results;
}

Entry VaultService::getEntry(EntryId id) const {
    requireUnlocked();
    const auto* entry = model_.findEntry(id);
    if (!entry) {
        throw ServiceException(ServiceError::EntryNotFound, "Entry not found");
    }
    return *entry;
}

EntryId VaultService::addEntry(const EntryDraft& draft) {
    requireUnlocked();

    Entry entry;
    entry.title = draft.title;
    entry.username = draft.username;
    entry.password = draft.password;
    entry.url = draft.url;
    entry.notes = draft.notes;
    entry.tags = draft.tags;
    entry.folderId = draft.folderId;
    entry.totp = draft.totp;
    entry.createdAt = std::chrono::system_clock::now();
    entry.modifiedAt = entry.createdAt;

    const auto id = model_.addEntry(std::move(entry));
    save();
    notifyChanged();
    return id;
}

void VaultService::updateEntry(EntryId id, const EntryDraft& draft) {
    requireUnlocked();

    const auto* existing = model_.findEntry(id);
    if (!existing) {
        throw ServiceException(ServiceError::EntryNotFound, "Entry not found");
    }

    Entry updated = *existing;
    updated.title = draft.title;
    updated.username = draft.username;
    updated.password = draft.password;
    updated.url = draft.url;
    updated.notes = draft.notes;
    updated.tags = draft.tags;
    updated.folderId = draft.folderId;
    updated.totp = draft.totp;
    updated.modifiedAt = std::chrono::system_clock::now();

    model_.updateEntry(updated);
    save();
    notifyChanged();
}

void VaultService::removeEntry(EntryId id) {
    requireUnlocked();
    if (!model_.removeEntry(id)) {
        throw ServiceException(ServiceError::EntryNotFound, "Entry not found");
    }
    save();
    notifyChanged();
}

std::string VaultService::currentTotpCode(EntryId id, std::chrono::system_clock::time_point now) const {
    requireUnlocked();
    const auto* entry = model_.findEntry(id);
    if (!entry) {
        throw ServiceException(ServiceError::EntryNotFound, "Entry not found");
    }
    if (!entry->totp) {
        throw ServiceException(ServiceError::InvalidArgument, "Entry has no TOTP configured");
    }
    const auto code = totp::totp(entry->totp->secret, entry->totp->params, now);
    return totp::formatCode(code, entry->totp->params.digits);
}

unsigned int VaultService::totpSecondsRemaining(EntryId id, std::chrono::system_clock::time_point now) const {
    requireUnlocked();
    const auto* entry = model_.findEntry(id);
    if (!entry) {
        throw ServiceException(ServiceError::EntryNotFound, "Entry not found");
    }
    if (!entry->totp) {
        throw ServiceException(ServiceError::InvalidArgument, "Entry has no TOTP configured");
    }
    return totp::secondsRemaining(entry->totp->params, now);
}

std::string VaultService::generatePassword(const util::PasswordGeneratorOptions& options) const {
    return util::generatePassword(options);
}

void VaultService::save() {
    requireUnlocked();
    const auto payload = model_.serialize();
    writeWithKey(path_, key_, salt_, kdfParams_, payload);
}

void VaultService::exportVaultTo(const std::filesystem::path& destPath) const {
    requireUnlocked();
    std::error_code ec;
    std::filesystem::copy_file(path_, destPath, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        throw ServiceException(ServiceError::IoError, "Failed to export vault file: " + ec.message());
    }
}

void VaultService::importVaultFrom(const std::filesystem::path& srcPath, std::string_view srcPassword,
                                    ImportMode mode) {
    requireUnlocked();

    VaultService imported;
    imported.unlock(srcPath, srcPassword); // propagates ServiceException on wrong password / corrupt file

    if (mode == ImportMode::Replace) {
        model_ = std::move(imported.model_);
    } else {
        for (const auto& [id, entry] : imported.model_.entries()) {
            Entry copy = entry;
            copy.id = 0;                 // model_.addEntry assigns a fresh id in this vault
            copy.folderId = std::nullopt; // folder ids aren't meaningful across vaults
            model_.addEntry(std::move(copy));
        }
    }

    save();
    notifyChanged();
}

int VaultService::onChanged(ChangeCallback callback) {
    const int token = nextListenerToken_++;
    listeners_.emplace_back(token, std::move(callback));
    return token;
}

void VaultService::removeChangeListener(int token) {
    listeners_.erase(
        std::remove_if(listeners_.begin(), listeners_.end(),
                        [token](const auto& entry) { return entry.first == token; }),
        listeners_.end());
}

} // namespace lusakey::core::vault
