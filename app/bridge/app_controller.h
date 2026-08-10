#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include <memory>
#include <string>

#include "lusakey/core/vault/vault_service.h"
#include "totp_list_model.h"

class VaultListModel;
class FolderListModel;
class QTimer;

// Thin Qt/QML-facing adapter over lusakey::core::vault::VaultService. All
// actual vault logic lives in VaultService (Qt-free, in libs/core) — this
// class only translates between Qt/QML idioms (signals, Q_INVOKABLE,
// QString) and VaultService's plain-C++ API, and owns the Qt-specific
// auto-lock / clipboard-clear timers.
//
// Simplification for this pass: the vault always lives at a single fixed
// path under the platform's app-data directory (see defaultVaultPath()) —
// there is no "choose a vault file location" flow yet. Export/import still
// take an explicit destination/source path (wiring those to a real QML
// FileDialog is a follow-up).
class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool unlocked READ isUnlocked NOTIFY unlockedChanged)
    Q_PROPERTY(bool lastUnlockFailed READ lastUnlockFailed NOTIFY lastUnlockFailedChanged)
    Q_PROPERTY(bool vaultExists READ vaultExists NOTIFY vaultExistsChanged)
    Q_PROPERTY(bool recoveryAvailable READ recoveryAvailable NOTIFY vaultExistsChanged)
    Q_PROPERTY(bool recoveryEnabled READ recoveryEnabled NOTIFY recoveryEnabledChanged)
    Q_PROPERTY(bool lastRecoveryFailed READ lastRecoveryFailed NOTIFY lastRecoveryFailedChanged)
    Q_PROPERTY(bool autoLockEnabled READ autoLockEnabled WRITE setAutoLockEnabled NOTIFY autoLockEnabledChanged)
    Q_PROPERTY(int autoLockMinutes READ autoLockMinutes WRITE setAutoLockMinutes NOTIFY autoLockMinutesChanged)
    Q_PROPERTY(bool clipboardClearEnabled READ clipboardClearEnabled WRITE setClipboardClearEnabled NOTIFY
                   clipboardClearEnabledChanged)
    Q_PROPERTY(int clipboardClearSeconds READ clipboardClearSeconds WRITE setClipboardClearSeconds NOTIFY
                   clipboardClearSecondsChanged)
    Q_PROPERTY(bool showWelcome READ showWelcome CONSTANT)
    Q_PROPERTY(QString currentFontFamily READ currentFontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(QStringList fontFamilies READ fontFamilies CONSTANT)

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    bool isUnlocked() const;
    bool lastUnlockFailed() const { return lastUnlockFailed_; }
    bool vaultExists() const;
    // Whether the on-disk vault at the default path has recovery questions
    // configured — checked WITHOUT unlocking (the unlock screen needs this
    // before the user has entered anything). False if no vault exists yet.
    bool recoveryAvailable() const;
    bool lastRecoveryFailed() const { return lastRecoveryFailed_; }
    // Whether the CURRENTLY UNLOCKED vault has recovery configured (for the
    // settings screen's "recovery: configured/not configured" indicator).
    // False whenever locked.
    bool recoveryEnabled() const;

    // Security preferences — persisted across runs via QSettings (see
    // AGENTS.md). Disabling either one is an explicit user choice: disabling
    // auto-lock means the vault simply never locks itself on idle;
    // disabling clipboard-clear means a copied password/TOTP code stays on
    // the clipboard until overwritten by something else.
    bool autoLockEnabled() const { return autoLockEnabled_; }
    int autoLockMinutes() const { return autoLockMinutes_; }
    bool clipboardClearEnabled() const { return clipboardClearEnabled_; }
    // Whether the first-launch onboarding (WelcomeScreen.qml) should be
    // shown — true until dismissWelcome() is called once, ever, persisted
    // via QSettings. Q_PROPERTY is CONSTANT: read once at startup by
    // Main.qml, not meant to change reactively mid-session.
    bool showWelcome() const;
    int clipboardClearSeconds() const { return clipboardClearSeconds_; }

    QString currentFontFamily() const;
    QStringList fontFamilies() const;
    Q_INVOKABLE void setFontFamily(const QString& fontFamily);

    VaultListModel* vaultListModel() const { return vaultListModel_.get(); }
    TotpListModel* totpListModel() const { return totpListModel_.get(); }
    FolderListModel* folderListModel() const { return folderListModel_.get(); }

public slots:
    void setAutoLockEnabled(bool enabled);
    void setAutoLockMinutes(int minutes);
    void setClipboardClearEnabled(bool enabled);
    void setClipboardClearSeconds(int seconds);
    // Persists that the onboarding has been seen, so showWelcome() returns
    // false on every future launch. Idempotent.
    void dismissWelcome();

    // Dispatches to createVault() or unlock() depending on vaultExists() —
    // the single entry point the unlock screen calls, so QML doesn't need
    // to know which case it is.
    void unlockOrCreate(const QString& masterPassword);
    void lock();

    // Unlocks the default vault using answers to its configured secret
    // questions instead of the master password. `answers` must be in the
    // same order recoveryQuestions() returned them.
    void unlockWithRecoveryAnswers(const QStringList& answers);
    // Returns the (plaintext, not secret) recovery questions configured for
    // the default vault — readable without unlocking. Empty if the vault
    // doesn't exist or has no recovery configured.
    QStringList recoveryQuestions() const;
    // Configures (or replaces) the currently-unlocked vault's recovery
    // slot. `answers[i]` corresponds to `questions[i]`; the count is
    // entirely up to the caller (the UI lets the user add/remove rows).
    void setupRecovery(const QStringList& questions, const QStringList& answers);
    void disableRecovery();
    // The "forgot password" escape hatch: permanently deletes the default
    // vault file so a new one can be created. Irreversible.
    void resetVault();

    // Restarts the auto-lock idle countdown. Called from mutating actions
    // below automatically, and from main.cpp's app-wide input event filter
    // (ActivityEventFilter) so moving the mouse/typing anywhere in the
    // window resets it too, not just explicit CRUD actions.
    void resetAutoLockTimer();

    void setSearchText(const QString& text);

    void addEntry(const QString& title, const QString& username, const QString& password, const QString& url,
                  const QString& notes);
    void updateEntry(qulonglong entryId, const QString& title, const QString& username, const QString& password,
                      const QString& url, const QString& notes);
    void removeEntry(qulonglong entryId);
    // Full entry fields for pre-filling the detail panel when opening an
    // EXISTING entry (keys: title/username/password/url/notes/hasTotp).
    // Empty map if entryId doesn't exist or the vault is locked.
    QVariantMap getEntry(qulonglong entryId);

    // Folder CRUD + per-entry assignment. folderId == 0 means "no folder"
    // wherever an entry's folder is being set/cleared (setEntryFolder) —
    // matches EntryFilter's "0 = unfiled" sentinel in libs/core.
    qulonglong addFolder(const QString& name);
    void renameFolder(qulonglong folderId, const QString& name);
    void removeFolder(qulonglong folderId);
    void setEntryFolder(qulonglong entryId, qulonglong folderId);
    // -1 = show all entries, 0 = unfiled only, >0 = that folder's entries.
    void setFolderFilter(qlonglong folderId);
    // One-shot {folderId, name} list for populating a picker (e.g. the entry
    // detail panel's folder combo box) without needing a live model there.
    QVariantList folderOptions();

    // Links entryId to a TOTP secret parsed from an otpauth://totp/... URI
    // (the format QR codes decode to — paste-in, no image import here; see
    // libs/qr for that, not yet wired to this method). Returns false (and
    // emits errorOccurred with the reason) if the URI is malformed or
    // entryId doesn't exist; the entry is left untouched either way.
    bool setEntryTotp(qulonglong entryId, const QString& otpauthUri);
    // Removes entryId's TOTP secret, if any. No-op if it has none.
    void removeEntryTotp(qulonglong entryId);
    // Creates a brand-new entry from an otpauth://totp/... URI alone (title
    // taken from the URI's issuer, falling back to its label) — the "add a
    // code without picking an existing entry first" path from
    // LinkTotpDialog. Returns false (and emits errorOccurred) if the URI is
    // malformed.
    bool addTotpEntry(const QString& otpauthUri);

    // Decodes a QR code from an image file and returns the otpauth://totp/...
    // URI found inside, or an empty string (+ errorOccurred) if the file
    // can't be opened, no QR code is found, or the image data is malformed.
    // Pure decode step — the caller (LinkTotpDialog.qml) feeds the result
    // into setEntryTotp()/addTotpEntry() exactly like a manually pasted URI.
    QString decodeTotpQrImage(const QUrl& fileUrl);

    QString currentTotpCode(qulonglong entryId);
    int currentTotpSecondsRemaining(qulonglong entryId);

    QString generatePassword(int length, bool includeUppercase, bool includeLowercase, bool includeDigits,
                              bool includeSymbols, bool excludeAmbiguous);
    int estimatePasswordStrength(const QString& password);

    void copyToClipboard(const QString& text);

    void exportVault(const QString& destPath);
    void importVault(const QString& srcPath, const QString& srcPassword, bool merge);

    void changeMasterPassword(const QString& oldPassword, const QString& newPassword);

private:
    void createVault(const QString& masterPassword);
    void unlock(const QString& masterPassword);
    void refreshVaultList();
    // Rebuilds the full set of TOTP-enabled entries (call alongside
    // refreshVaultList() wherever entries are added/removed/updated/
    // unlocked/locked). tickTotpList(), by contrast, only refreshes the
    // current code/countdown for that same set every second.
    void refreshTotpList();
    void refreshFolderList();
    void tickTotpList();
    TotpListModel::Row buildTotpRow(lusakey::core::vault::EntryId id, const std::string& title) const;
    QString defaultVaultPath() const;

signals:
    void unlockedChanged();
    void lastUnlockFailedChanged();
    void vaultExistsChanged();
    void recoveryEnabledChanged();
    void lastRecoveryFailedChanged();
    void autoLockEnabledChanged();
    void autoLockMinutesChanged();
    void clipboardClearEnabledChanged();
    void clipboardClearSecondsChanged();
    void fontFamilyChanged();
    void unlocked();
    void locked();
    void errorOccurred(const QString& message);

private:
    lusakey::core::vault::VaultService service_;
    std::unique_ptr<VaultListModel> vaultListModel_;
    std::unique_ptr<TotpListModel> totpListModel_;
    std::unique_ptr<FolderListModel> folderListModel_;
    bool lastUnlockFailed_ = false;
    bool lastRecoveryFailed_ = false;
    QString searchText_;
    qlonglong folderFilter_ = -1;

    QTimer* autoLockTimer_ = nullptr;
    QTimer* clipboardClearTimer_ = nullptr;
    QString clipboardGuardText_;
    QTimer* totpTickTimer_ = nullptr;

    QSettings settings_;
    bool autoLockEnabled_ = true;
    int autoLockMinutes_ = 5;
    bool clipboardClearEnabled_ = true;
    int clipboardClearSeconds_ = 20;
    QString currentFontFamily_;
    QStringList fontFamilies_;
};
