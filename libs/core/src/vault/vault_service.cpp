#include "lusakey/core/vault/vault_service.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <system_error>
#include <utility>

#include "lusakey/core/crypto/random.h"
#include "lusakey/core/totp/totp.h"

namespace lusakey::core::vault {

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Strips ASCII whitespace and lowercases ASCII letters so answers like
// " Fluffy " and "fluffy" match. Note: std::tolower only affects the ASCII
// range — non-ASCII (e.g. Cyrillic) answers are trimmed but not
// case-folded, so "Москва" and "москва" are treated as different answers.
// Full Unicode case folding would need an extra dependency (ICU); not worth
// it for this feature.
std::string normalizeAnswer(const std::string& answer) {
    std::string result;
    result.reserve(answer.size());
    for (const unsigned char c : answer) {
        if (!std::isspace(c)) {
            result.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    return result;
}

std::string joinNormalizedAnswers(const std::vector<std::string>& answers) {
    std::string joined;
    for (const auto& a : answers) {
        joined += normalizeAnswer(a);
        joined.push_back('\x1F'); // unit separator; cheap delimiter, answers can't contain a control char accidentally colliding often
    }
    return joined;
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

void VaultService::finishUnlock(const std::filesystem::path& path, RawVaultFile&& raw, crypto::SecureBuffer&& dek,
                                 const std::vector<std::uint8_t>& payload) {
    VaultModel model;
    try {
        model = VaultModel::deserialize(payload);
    } catch (const std::exception& e) {
        throw ServiceException(ServiceError::FileCorrupt, e.what());
    }

    model_ = std::move(model);
    path_ = path;
    dek_ = std::move(dek);
    passwordSlot_ = raw.header.passwordSlot;
    recoveryEnabled_ = raw.header.recoveryEnabled;
    recoveryQuestions_ = raw.header.recoveryQuestions;
    recoverySlot_ = raw.header.recoverySlot;
    unlocked_ = true;
}

void VaultService::createVault(const std::filesystem::path& path, std::string_view masterPassword,
                                const crypto::KdfParams& kdfParams) {
    if (masterPassword.empty()) {
        throw ServiceException(ServiceError::InvalidArgument, "Master password must not be empty");
    }

    path_ = path;
    model_ = VaultModel{};

    dek_ = crypto::SecureBuffer(crypto::kDerivedKeyBytes);
    crypto::randomBytes(dek_.data(), dek_.size());
    passwordSlot_ = makeKeySlot(dek_, masterPassword, kdfParams);
    recoveryEnabled_ = false;
    recoveryQuestions_.clear();
    recoverySlot_ = KeySlot{};

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

    crypto::SecureBuffer dek(0);
    try {
        dek = unwrapDek(raw.header.passwordSlot, masterPassword);
    } catch (const VaultFileException&) {
        throw ServiceException(ServiceError::WrongPassword, "Incorrect master password");
    }

    std::vector<std::uint8_t> payload;
    try {
        payload = decryptBody(raw, dek);
    } catch (const VaultFileException& e) {
        throw ServiceException(ServiceError::FileCorrupt, e.what());
    }

    finishUnlock(path, std::move(raw), std::move(dek), payload);
}

void VaultService::unlockWithRecoveryAnswers(const std::filesystem::path& path, const std::vector<std::string>& answers) {
    RawVaultFile raw;
    try {
        raw = readRaw(path);
    } catch (const VaultFileException& e) {
        if (e.code() == VaultFileError::NotFound) {
            throw ServiceException(ServiceError::FileNotFound, e.what());
        }
        throw ServiceException(ServiceError::FileCorrupt, e.what());
    }

    if (!raw.header.recoveryEnabled) {
        throw ServiceException(ServiceError::RecoveryNotConfigured, "This vault has no recovery method configured");
    }

    const auto joined = joinNormalizedAnswers(answers);
    crypto::SecureBuffer dek(0);
    try {
        dek = unwrapDek(raw.header.recoverySlot, joined);
    } catch (const VaultFileException&) {
        throw ServiceException(ServiceError::WrongAnswers, "One or more answers are incorrect");
    }

    std::vector<std::uint8_t> payload;
    try {
        payload = decryptBody(raw, dek);
    } catch (const VaultFileException& e) {
        throw ServiceException(ServiceError::FileCorrupt, e.what());
    }

    finishUnlock(path, std::move(raw), std::move(dek), payload);
}

void VaultService::lock() {
    model_ = VaultModel{};
    dek_.zero();
    dek_ = crypto::SecureBuffer(0);
    passwordSlot_ = KeySlot{};
    recoveryEnabled_ = false;
    recoveryQuestions_.clear();
    recoverySlot_ = KeySlot{};
    unlocked_ = false;
}

void VaultService::changeMasterPassword(std::string_view oldPassword, std::string_view newPassword) {
    requireUnlocked();

    crypto::SecureBuffer recovered(0);
    try {
        recovered = unwrapDek(passwordSlot_, oldPassword);
    } catch (const VaultFileException&) {
        throw ServiceException(ServiceError::WrongPassword, "Current password is incorrect");
    }
    // Belt-and-suspenders: unwrapDek succeeding already proves `oldPassword`
    // is correct for this slot, but confirm it recovered the DEK actually in
    // use (should be true by construction — this only guards against a bug
    // leaving passwordSlot_ out of sync with dek_).
    if (recovered.size() != dek_.size() || std::memcmp(recovered.data(), dek_.data(), dek_.size()) != 0) {
        throw ServiceException(ServiceError::WrongPassword, "Current password is incorrect");
    }
    if (newPassword.empty()) {
        throw ServiceException(ServiceError::InvalidArgument, "New password must not be empty");
    }

    passwordSlot_ = makeKeySlot(dek_, newPassword, passwordSlot_.kdfParams);
    save();
}

void VaultService::setupRecovery(const std::vector<std::string>& questions, const std::vector<std::string>& answers,
                                  const crypto::KdfParams& kdfParams) {
    requireUnlocked();
    if (questions.empty() || questions.size() != answers.size()) {
        throw ServiceException(ServiceError::InvalidArgument,
                                "Need at least one question, and exactly one answer per question");
    }
    for (const auto& q : questions) {
        if (q.empty()) {
            throw ServiceException(ServiceError::InvalidArgument, "Questions must not be empty");
        }
    }
    for (const auto& a : answers) {
        if (normalizeAnswer(a).empty()) {
            throw ServiceException(ServiceError::InvalidArgument, "Answers must not be empty");
        }
    }

    const auto joined = joinNormalizedAnswers(answers);
    recoverySlot_ = makeKeySlot(dek_, joined, kdfParams);
    recoveryEnabled_ = true;
    recoveryQuestions_ = questions;
    save();
}

void VaultService::disableRecovery() {
    requireUnlocked();
    recoveryEnabled_ = false;
    recoveryQuestions_.clear();
    recoverySlot_ = KeySlot{};
    save();
}

bool VaultService::hasRecovery(const std::filesystem::path& path) {
    try {
        return readRaw(path).header.recoveryEnabled;
    } catch (const VaultFileException&) {
        return false;
    }
}

std::vector<std::string> VaultService::getRecoveryQuestions(const std::filesystem::path& path) {
    try {
        const auto raw = readRaw(path);
        return raw.header.recoveryEnabled ? raw.header.recoveryQuestions : std::vector<std::string>{};
    } catch (const VaultFileException&) {
        return {};
    }
}

void VaultService::resetVault(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec) {
        throw ServiceException(ServiceError::IoError, "Failed to delete vault file: " + ec.message());
    }
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
    writeVault(path_, dek_, passwordSlot_, recoveryEnabled_, recoveryQuestions_, recoverySlot_, payload);
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
