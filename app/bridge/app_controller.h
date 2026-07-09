#pragma once

#include <QObject>
#include <QString>

#include <memory>

#include "lusakey/core/vault/vault_service.h"

class VaultListModel;
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

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    bool isUnlocked() const;
    bool lastUnlockFailed() const { return lastUnlockFailed_; }
    bool vaultExists() const;

    VaultListModel* vaultListModel() const { return vaultListModel_.get(); }

public slots:
    // Dispatches to createVault() or unlock() depending on vaultExists() —
    // the single entry point the unlock screen calls, so QML doesn't need
    // to know which case it is.
    void unlockOrCreate(const QString& masterPassword);
    void lock();

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
    QString defaultVaultPath() const;

signals:
    void unlockedChanged();
    void lastUnlockFailedChanged();
    void vaultExistsChanged();
    void unlocked();
    void locked();
    void errorOccurred(const QString& message);

private:
    lusakey::core::vault::VaultService service_;
    std::unique_ptr<VaultListModel> vaultListModel_;
    bool lastUnlockFailed_ = false;
    QString searchText_;

    QTimer* autoLockTimer_ = nullptr;
    QTimer* clipboardClearTimer_ = nullptr;
    QString clipboardGuardText_;
};
