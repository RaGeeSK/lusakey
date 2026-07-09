#include "app_controller.h"

#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QTimer>

#include "lusakey/core/util/password_generator.h"
#include "vault_list_model.h"

using lusakey::core::vault::EntryDraft;
using lusakey::core::vault::EntryFilter;
using lusakey::core::vault::EntryId;
using lusakey::core::vault::ImportMode;
using lusakey::core::vault::ServiceException;

namespace {
constexpr int kDefaultAutoLockMinutes = 5;
constexpr int kDefaultClipboardClearSeconds = 20;
} // namespace

AppController::AppController(QObject* parent)
    : QObject(parent), vaultListModel_(std::make_unique<VaultListModel>()) {
    autoLockTimer_ = new QTimer(this);
    autoLockTimer_->setInterval(kDefaultAutoLockMinutes * 60 * 1000);
    autoLockTimer_->setSingleShot(true);
    connect(autoLockTimer_, &QTimer::timeout, this, &AppController::lock);

    clipboardClearTimer_ = new QTimer(this);
    clipboardClearTimer_->setSingleShot(true);
    connect(clipboardClearTimer_, &QTimer::timeout, this, [this]() {
        auto* clipboard = QGuiApplication::clipboard();
        if (clipboard && clipboard->text() == clipboardGuardText_) {
            clipboard->clear();
        }
    });
}

AppController::~AppController() = default;

bool AppController::isUnlocked() const {
    return service_.isUnlocked();
}

QString AppController::defaultVaultPath() const {
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/vault.lusakey");
}

bool AppController::vaultExists() const {
    return QFileInfo::exists(defaultVaultPath());
}

void AppController::unlockOrCreate(const QString& masterPassword) {
    if (vaultExists()) {
        unlock(masterPassword);
    } else {
        createVault(masterPassword);
    }
}

void AppController::createVault(const QString& masterPassword) {
    try {
        service_.createVault(defaultVaultPath().toStdString(), masterPassword.toStdString());
        lastUnlockFailed_ = false;
        emit lastUnlockFailedChanged();
        emit vaultExistsChanged();
        emit unlockedChanged();
        emit unlocked();
        refreshVaultList();
        autoLockTimer_->start();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::unlock(const QString& masterPassword) {
    try {
        service_.unlock(defaultVaultPath().toStdString(), masterPassword.toStdString());
        lastUnlockFailed_ = false;
        emit lastUnlockFailedChanged();
        emit unlockedChanged();
        emit unlocked();
        refreshVaultList();
        autoLockTimer_->start();
    } catch (const ServiceException& e) {
        lastUnlockFailed_ = true;
        emit lastUnlockFailedChanged();
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::lock() {
    service_.lock();
    autoLockTimer_->stop();
    emit unlockedChanged();
    emit locked();
    refreshVaultList();
}

void AppController::resetAutoLockTimer() {
    if (service_.isUnlocked()) {
        autoLockTimer_->start();
    }
}

void AppController::setSearchText(const QString& text) {
    searchText_ = text;
    refreshVaultList();
}

void AppController::addEntry(const QString& title, const QString& username, const QString& password,
                              const QString& url, const QString& notes) {
    autoLockTimer_->start();
    try {
        EntryDraft draft;
        draft.title = title.toStdString();
        draft.username = username.toStdString();
        draft.password = password.toStdString();
        draft.url = url.toStdString();
        draft.notes = notes.toStdString();
        service_.addEntry(draft);
        refreshVaultList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::updateEntry(qulonglong entryId, const QString& title, const QString& username,
                                 const QString& password, const QString& url, const QString& notes) {
    autoLockTimer_->start();
    try {
        EntryDraft draft;
        draft.title = title.toStdString();
        draft.username = username.toStdString();
        draft.password = password.toStdString();
        draft.url = url.toStdString();
        draft.notes = notes.toStdString();
        service_.updateEntry(static_cast<EntryId>(entryId), draft);
        refreshVaultList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::removeEntry(qulonglong entryId) {
    autoLockTimer_->start();
    try {
        service_.removeEntry(static_cast<EntryId>(entryId));
        refreshVaultList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

QString AppController::currentTotpCode(qulonglong entryId) {
    try {
        return QString::fromStdString(service_.currentTotpCode(static_cast<EntryId>(entryId)));
    } catch (const ServiceException&) {
        return {};
    }
}

int AppController::currentTotpSecondsRemaining(qulonglong entryId) {
    try {
        return static_cast<int>(service_.totpSecondsRemaining(static_cast<EntryId>(entryId)));
    } catch (const ServiceException&) {
        return 0;
    }
}

QString AppController::generatePassword(int length, bool includeUppercase, bool includeLowercase,
                                         bool includeDigits, bool includeSymbols, bool excludeAmbiguous) {
    lusakey::core::util::PasswordGeneratorOptions options;
    options.length = static_cast<unsigned int>(length);
    options.includeUppercase = includeUppercase;
    options.includeLowercase = includeLowercase;
    options.includeDigits = includeDigits;
    options.includeSymbols = includeSymbols;
    options.excludeAmbiguous = excludeAmbiguous;
    try {
        return QString::fromStdString(service_.generatePassword(options));
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return {};
    }
}

int AppController::estimatePasswordStrength(const QString& password) {
    return lusakey::core::util::estimatePasswordStrength(password.toStdString());
}

void AppController::copyToClipboard(const QString& text) {
    auto* clipboard = QGuiApplication::clipboard();
    if (!clipboard) {
        return;
    }
    clipboard->setText(text);
    clipboardGuardText_ = text;
    clipboardClearTimer_->start(kDefaultClipboardClearSeconds * 1000);
}

void AppController::exportVault(const QString& destPath) {
    try {
        service_.exportVaultTo(destPath.toStdString());
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::importVault(const QString& srcPath, const QString& srcPassword, bool merge) {
    try {
        service_.importVaultFrom(srcPath.toStdString(), srcPassword.toStdString(),
                                  merge ? ImportMode::Merge : ImportMode::Replace);
        refreshVaultList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::changeMasterPassword(const QString& oldPassword, const QString& newPassword) {
    try {
        service_.changeMasterPassword(oldPassword.toStdString(), newPassword.toStdString());
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::refreshVaultList() {
    EntryFilter filter;
    filter.searchText = searchText_.toStdString();
    vaultListModel_->setEntries(service_.isUnlocked() ? service_.listEntries(filter)
                                                       : std::vector<lusakey::core::vault::EntrySummary>{});
}
