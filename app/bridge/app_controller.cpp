#include "app_controller.h"

#include <algorithm>

#include <QClipboard>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QImage>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

#include "lusakey/core/totp/otpauth_uri.h"
#include "lusakey/core/util/password_generator.h"
#include "lusakey/qr/qr_decoder.h"
#include "folder_list_model.h"
#include "vault_list_model.h"

using lusakey::core::vault::EntryDraft;
using lusakey::core::vault::EntryFilter;
using lusakey::core::vault::EntryId;
using lusakey::core::vault::Folder;
using lusakey::core::vault::FolderId;
using lusakey::core::vault::ImportMode;
using lusakey::core::vault::ServiceException;
using lusakey::core::vault::TotpSpec;

namespace {
constexpr int kDefaultAutoLockMinutes = 5;
constexpr int kDefaultClipboardClearSeconds = 20;

QString browserLoginDirectory() {
    const auto testDir = qEnvironmentVariable("LUSAKEY_TEST_VAULT_DIR");
    const auto baseDir = testDir.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : testDir;
    return baseDir + QStringLiteral("/browser-login");
}
} // namespace

AppController::AppController(QObject* parent)
    : QObject(parent),
      vaultListModel_(std::make_unique<VaultListModel>()),
      totpListModel_(std::make_unique<TotpListModel>()),
      folderListModel_(std::make_unique<FolderListModel>()) {
    settings_.beginGroup(QStringLiteral("security"));
    autoLockEnabled_ = settings_.value(QStringLiteral("autoLockEnabled"), true).toBool();
    autoLockMinutes_ = settings_.value(QStringLiteral("autoLockMinutes"), kDefaultAutoLockMinutes).toInt();
    clipboardClearEnabled_ = settings_.value(QStringLiteral("clipboardClearEnabled"), true).toBool();
    clipboardClearSeconds_ =
        settings_.value(QStringLiteral("clipboardClearSeconds"), kDefaultClipboardClearSeconds).toInt();
    settings_.endGroup();

    settings_.beginGroup(QStringLiteral("appearance"));
    currentFontFamily_ = settings_.value(QStringLiteral("fontFamily"), QStringLiteral("Inter")).toString();
    settings_.endGroup();

    const QFontDatabase fontDatabase;
    fontFamilies_ = fontDatabase.families();

    autoLockTimer_ = new QTimer(this);
    autoLockTimer_->setInterval(autoLockMinutes_ * 60 * 1000);
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

    // Always running (not gated on unlocked/lock state) — tickTotpList()
    // itself no-ops when there's nothing to update, same as how the app-wide
    // input filter runs regardless of lock state.
    totpTickTimer_ = new QTimer(this);
    totpTickTimer_->setInterval(1000);
    connect(totpTickTimer_, &QTimer::timeout, this, &AppController::tickTotpList);
    totpTickTimer_->start();
}

AppController::~AppController() = default;

bool AppController::isUnlocked() const {
    return service_.isUnlocked();
}

QString AppController::defaultVaultPath() const {
    // Dev/test aid: redirects the vault to an isolated throwaway directory
    // instead of the real profile path, so UI changes can be visually
    // verified (screenshots, scripted clicks) without ever touching a
    // real vault. No-op unless explicitly set — see AGENTS.md.
    const auto override = qEnvironmentVariable("LUSAKEY_TEST_VAULT_DIR");
    if (!override.isEmpty()) {
        QDir().mkpath(override);
        return override + QStringLiteral("/vault.lusakey");
    }
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/vault.lusakey");
}

bool AppController::vaultExists() const {
    return QFileInfo::exists(defaultVaultPath());
}

bool AppController::showWelcome() const {
    return !settings_.value(QStringLiteral("ui/welcomeShown"), false).toBool();
}

void AppController::dismissWelcome() {
    settings_.setValue(QStringLiteral("ui/welcomeShown"), true);
}

bool AppController::recoveryAvailable() const {
    return lusakey::core::vault::VaultService::hasRecovery(defaultVaultPath().toStdString());
}

bool AppController::recoveryEnabled() const {
    return service_.isUnlocked() && service_.recoveryEnabled();
}

QStringList AppController::recoveryQuestions() const {
    const auto questions = lusakey::core::vault::VaultService::getRecoveryQuestions(defaultVaultPath().toStdString());
    QStringList result;
    result.reserve(static_cast<qsizetype>(questions.size()));
    for (const auto& q : questions) {
        result.append(QString::fromStdString(q));
    }
    return result;
}

void AppController::setAutoLockEnabled(bool enabled) {
    if (autoLockEnabled_ == enabled) {
        return;
    }
    autoLockEnabled_ = enabled;
    settings_.setValue(QStringLiteral("security/autoLockEnabled"), autoLockEnabled_);
    emit autoLockEnabledChanged();
    resetAutoLockTimer(); // starts it if now enabled+unlocked, or leaves it be otherwise
    if (!autoLockEnabled_) {
        autoLockTimer_->stop();
    }
}

void AppController::setAutoLockMinutes(int minutes) {
    if (minutes <= 0 || autoLockMinutes_ == minutes) {
        return;
    }
    autoLockMinutes_ = minutes;
    settings_.setValue(QStringLiteral("security/autoLockMinutes"), autoLockMinutes_);
    emit autoLockMinutesChanged();
    autoLockTimer_->setInterval(autoLockMinutes_ * 60 * 1000);
    if (autoLockTimer_->isActive()) {
        autoLockTimer_->start(); // restart with the new interval
    }
}

void AppController::setClipboardClearEnabled(bool enabled) {
    if (clipboardClearEnabled_ == enabled) {
        return;
    }
    clipboardClearEnabled_ = enabled;
    settings_.setValue(QStringLiteral("security/clipboardClearEnabled"), clipboardClearEnabled_);
    emit clipboardClearEnabledChanged();
    if (!clipboardClearEnabled_) {
        clipboardClearTimer_->stop();
    }
}

void AppController::setClipboardClearSeconds(int seconds) {
    if (seconds <= 0 || clipboardClearSeconds_ == seconds) {
        return;
    }
    clipboardClearSeconds_ = seconds;
    settings_.setValue(QStringLiteral("security/clipboardClearSeconds"), clipboardClearSeconds_);
    emit clipboardClearSecondsChanged();
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
        refreshTotpList();
        refreshFolderList();
        resetAutoLockTimer();
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
        refreshTotpList();
        refreshFolderList();
        resetAutoLockTimer();
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
    emit recoveryEnabledChanged();
    emit locked();
    refreshVaultList();
    refreshTotpList();
    refreshFolderList();
}

void AppController::unlockWithRecoveryAnswers(const QStringList& answers) {
    std::vector<std::string> converted;
    converted.reserve(static_cast<std::size_t>(answers.size()));
    for (const auto& a : answers) {
        converted.push_back(a.toStdString());
    }

    try {
        service_.unlockWithRecoveryAnswers(defaultVaultPath().toStdString(), converted);
        lastRecoveryFailed_ = false;
        emit lastRecoveryFailedChanged();
        lastUnlockFailed_ = false;
        emit lastUnlockFailedChanged();
        emit unlockedChanged();
        emit recoveryEnabledChanged();
        emit unlocked();
        refreshVaultList();
        refreshTotpList();
        refreshFolderList();
        resetAutoLockTimer();
    } catch (const ServiceException& e) {
        lastRecoveryFailed_ = true;
        emit lastRecoveryFailedChanged();
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::setupRecovery(const QStringList& questions, const QStringList& answers) {
    std::vector<std::string> convertedQuestions;
    std::vector<std::string> convertedAnswers;
    convertedQuestions.reserve(static_cast<std::size_t>(questions.size()));
    convertedAnswers.reserve(static_cast<std::size_t>(answers.size()));
    for (const auto& q : questions) {
        convertedQuestions.push_back(q.toStdString());
    }
    for (const auto& a : answers) {
        convertedAnswers.push_back(a.toStdString());
    }

    try {
        service_.setupRecovery(convertedQuestions, convertedAnswers);
        emit recoveryEnabledChanged();
        emit vaultExistsChanged(); // recoveryAvailable shares this NOTIFY signal
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::disableRecovery() {
    try {
        service_.disableRecovery();
        emit recoveryEnabledChanged();
        emit vaultExistsChanged();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::resetVault() {
    try {
        lusakey::core::vault::VaultService::resetVault(defaultVaultPath().toStdString());
        if (service_.isUnlocked()) {
            service_.lock();
        }
        autoLockTimer_->stop();
        lastUnlockFailed_ = false;
        lastRecoveryFailed_ = false;
        emit lastUnlockFailedChanged();
        emit lastRecoveryFailedChanged();
        emit unlockedChanged();
        emit vaultExistsChanged();
        emit recoveryEnabledChanged();
        emit locked(); // sends the UI back to the unlock/create screen
        refreshVaultList();
        refreshTotpList();
        refreshFolderList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::resetAutoLockTimer() {
    if (service_.isUnlocked() && autoLockEnabled_) {
        autoLockTimer_->start();
    }
}

void AppController::setSearchText(const QString& text) {
    searchText_ = text;
    refreshVaultList();
}

void AppController::addEntry(const QString& title, const QString& username, const QString& password,
                              const QString& url, const QString& notes) {
    resetAutoLockTimer();
    try {
        EntryDraft draft;
        draft.title = title.toStdString();
        draft.username = username.toStdString();
        draft.password = password.toStdString();
        draft.url = url.toStdString();
        draft.notes = notes.toStdString();
        service_.addEntry(draft);
        refreshVaultList();
        refreshTotpList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::updateEntry(qulonglong entryId, const QString& title, const QString& username,
                                 const QString& password, const QString& url, const QString& notes) {
    resetAutoLockTimer();
    try {
        const auto id = static_cast<EntryId>(entryId);
        // Carry forward fields this method's caller (the detail panel) never
        // touches — tags/folderId/totp — so saving an edit doesn't silently
        // wipe a TOTP secret linked via setEntryTotp().
        const auto existing = service_.getEntry(id);
        EntryDraft draft;
        draft.title = title.toStdString();
        draft.username = username.toStdString();
        draft.password = password.toStdString();
        draft.url = url.toStdString();
        draft.notes = notes.toStdString();
        draft.tags = existing.tags;
        draft.folderId = existing.folderId;
        draft.totp = existing.totp;
        service_.updateEntry(id, draft);
        refreshVaultList();
        refreshTotpList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::removeEntry(qulonglong entryId) {
    resetAutoLockTimer();
    try {
        service_.removeEntry(static_cast<EntryId>(entryId));
        refreshVaultList();
        refreshTotpList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

QVariantMap AppController::getEntry(qulonglong entryId) {
    try {
        const auto entry = service_.getEntry(static_cast<EntryId>(entryId));
        QVariantMap result;
        result[QStringLiteral("title")] = QString::fromStdString(entry.title);
        result[QStringLiteral("username")] = QString::fromStdString(entry.username);
        result[QStringLiteral("password")] = QString::fromStdString(entry.password);
        result[QStringLiteral("url")] = QString::fromStdString(entry.url);
        result[QStringLiteral("notes")] = QString::fromStdString(entry.notes);
        result[QStringLiteral("hasTotp")] = entry.totp.has_value();
        result[QStringLiteral("folderId")] = entry.folderId.has_value() ? static_cast<qulonglong>(*entry.folderId) : 0ULL;
        return result;
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return {};
    }
}

qulonglong AppController::addFolder(const QString& name) {
    resetAutoLockTimer();
    try {
        const auto id = service_.addFolder(name.toStdString());
        refreshFolderList();
        return static_cast<qulonglong>(id);
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return 0;
    }
}

void AppController::renameFolder(qulonglong folderId, const QString& name) {
    resetAutoLockTimer();
    try {
        service_.renameFolder(static_cast<FolderId>(folderId), name.toStdString());
        refreshFolderList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::removeFolder(qulonglong folderId) {
    resetAutoLockTimer();
    try {
        service_.removeFolder(static_cast<FolderId>(folderId));
        // The removed folder may have been what the list was filtered on,
        // and entries referencing it were just orphaned server-side.
        if (folderFilter_ == static_cast<qlonglong>(folderId)) {
            folderFilter_ = -1;
        }
        refreshFolderList();
        refreshVaultList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::setEntryFolder(qulonglong entryId, qulonglong folderId) {
    resetAutoLockTimer();
    try {
        const auto id = static_cast<EntryId>(entryId);
        const auto existing = service_.getEntry(id);
        EntryDraft draft;
        draft.title = existing.title;
        draft.username = existing.username;
        draft.password = existing.password;
        draft.url = existing.url;
        draft.notes = existing.notes;
        draft.tags = existing.tags;
        draft.totp = existing.totp;
        draft.folderId = folderId == 0 ? std::nullopt : std::optional<FolderId>(static_cast<FolderId>(folderId));
        service_.updateEntry(id, draft);
        refreshVaultList();
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AppController::setFolderFilter(qlonglong folderId) {
    folderFilter_ = folderId;
    refreshVaultList();
}

QVariantList AppController::folderOptions() {
    QVariantList result;
    if (!service_.isUnlocked()) {
        return result;
    }
    for (const auto& folder : service_.listFolders()) {
        QVariantMap entry;
        entry[QStringLiteral("folderId")] = static_cast<qulonglong>(folder.id);
        entry[QStringLiteral("name")] = QString::fromStdString(folder.name);
        result.append(entry);
    }
    return result;
}

bool AppController::setEntryTotp(qulonglong entryId, const QString& otpauthUri) {
    resetAutoLockTimer();
    try {
        const auto id = static_cast<EntryId>(entryId);
        const auto existing = service_.getEntry(id);

        lusakey::core::totp::OtpAuthUri parsed;
        try {
            parsed = lusakey::core::totp::parseOtpAuthUri(otpauthUri.toStdString());
        } catch (const std::invalid_argument& e) {
            emit errorOccurred(QString::fromStdString(e.what()));
            return false;
        }

        EntryDraft draft;
        draft.title = existing.title;
        draft.username = existing.username;
        draft.password = existing.password;
        draft.url = existing.url;
        draft.notes = existing.notes;
        draft.tags = existing.tags;
        draft.folderId = existing.folderId;
        draft.totp = TotpSpec{parsed.secret, parsed.params};
        service_.updateEntry(id, draft);
        refreshVaultList();
        refreshTotpList();
        return true;
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool AppController::addTotpEntry(const QString& otpauthUri) {
    resetAutoLockTimer();
    lusakey::core::totp::OtpAuthUri parsed;
    try {
        parsed = lusakey::core::totp::parseOtpAuthUri(otpauthUri.toStdString());
    } catch (const std::invalid_argument& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }

    QString title = QString::fromStdString(parsed.issuer);
    if (title.isEmpty()) {
        title = QString::fromStdString(parsed.label);
    }
    if (title.isEmpty()) {
        title = tr("Код авторизации");
    }

    try {
        EntryDraft draft;
        draft.title = title.toStdString();
        draft.totp = TotpSpec{parsed.secret, parsed.params};
        service_.addEntry(draft);
        refreshVaultList();
        refreshTotpList();
        return true;
    } catch (const ServiceException& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

QString AppController::decodeTotpQrImage(const QUrl& fileUrl) {
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    QImage image(path);
    if (image.isNull()) {
        emit errorOccurred(tr("Не удалось открыть файл изображения"));
        return {};
    }
    const QImage gray = image.convertToFormat(QImage::Format_Grayscale8);

    lusakey::core::qr::GrayscaleImage grayscale;
    grayscale.width = gray.width();
    grayscale.height = gray.height();
    grayscale.pixels.resize(static_cast<std::size_t>(grayscale.width) * static_cast<std::size_t>(grayscale.height));
    // QImage scanlines are stride-padded (bytesPerLine() >= width for
    // Format_Grayscale8) — copy row by row, not a single memcpy of the
    // whole buffer, to match GrayscaleImage's documented tightly-packed
    // (no padding) layout.
    for (int y = 0; y < grayscale.height; ++y) {
        const uchar* line = gray.constScanLine(y);
        std::copy_n(line, grayscale.width,
                    grayscale.pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(grayscale.width));
    }

    try {
        const auto uri = lusakey::core::qr::decode(grayscale);
        if (!uri.has_value()) {
            emit errorOccurred(tr("QR-код не найден на изображении"));
            return {};
        }
        return QString::fromStdString(*uri);
    } catch (const std::runtime_error& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
        return {};
    }
}

void AppController::removeEntryTotp(qulonglong entryId) {
    resetAutoLockTimer();
    try {
        const auto id = static_cast<EntryId>(entryId);
        const auto existing = service_.getEntry(id);
        if (!existing.totp.has_value()) {
            return;
        }
        EntryDraft draft;
        draft.title = existing.title;
        draft.username = existing.username;
        draft.password = existing.password;
        draft.url = existing.url;
        draft.notes = existing.notes;
        draft.tags = existing.tags;
        draft.folderId = existing.folderId;
        draft.totp = std::nullopt;
        service_.updateEntry(id, draft);
        refreshVaultList();
        refreshTotpList();
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
    if (clipboardClearEnabled_) {
        clipboardGuardText_ = text;
        clipboardClearTimer_->start(clipboardClearSeconds_ * 1000);
    }
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
        refreshTotpList();
        refreshFolderList();
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
    if (folderFilter_ >= 0) {
        filter.folderId = static_cast<FolderId>(folderFilter_);
    }
    vaultListModel_->setEntries(service_.isUnlocked() ? service_.listEntries(filter)
                                                       : std::vector<lusakey::core::vault::EntrySummary>{});
}

void AppController::refreshFolderList() {
    folderListModel_->setFolders(service_.isUnlocked() ? service_.listFolders() : std::vector<Folder>{});
}

TotpListModel::Row AppController::buildTotpRow(EntryId id, const std::string& title) const {
    TotpListModel::Row row;
    row.entryId = static_cast<qulonglong>(id);
    row.title = QString::fromStdString(title);
    try {
        row.code = QString::fromStdString(service_.currentTotpCode(id));
        row.secondsRemaining = static_cast<int>(service_.totpSecondsRemaining(id));
    } catch (const ServiceException&) {
        row.secondsRemaining = 0;
    }
    return row;
}

void AppController::refreshTotpList() {
    // Deliberately ignores searchText_ — the Authenticator Codes tab shows
    // every TOTP-enabled entry regardless of what's typed into the entries
    // list's search box (they're independent views).
    std::vector<TotpListModel::Row> rows;
    if (service_.isUnlocked()) {
        for (const auto& summary : service_.listEntries(EntryFilter{})) {
            if (summary.hasTotp) {
                rows.push_back(buildTotpRow(summary.id, summary.title));
            }
        }
    }
    totpListModel_->setRows(std::move(rows));
}

void AppController::tickTotpList() {
    const auto& currentRows = totpListModel_->rows();
    if (currentRows.empty()) {
        return;
    }
    std::vector<TotpListModel::Row> updated;
    updated.reserve(currentRows.size());
    for (const auto& row : currentRows) {
        updated.push_back(buildTotpRow(static_cast<EntryId>(row.entryId), row.title.toStdString()));
    }
    totpListModel_->updateTiming(updated);
}

QString AppController::currentFontFamily() const {
    return currentFontFamily_;
}

QStringList AppController::fontFamilies() const {
    return fontFamilies_;
}

void AppController::setFontFamily(const QString& fontFamily) {
    if (!fontFamilies_.contains(fontFamily) || currentFontFamily_ == fontFamily) {
        return;
    }

    currentFontFamily_ = fontFamily;
    settings_.setValue(QStringLiteral("appearance/fontFamily"), fontFamily);
    emit fontFamilyChanged();
}

void AppController::beginBrowserLogin(const QString& token) {
    static const QRegularExpression tokenPattern(QStringLiteral("^[0-9a-f]{32}$"));
    if (!tokenPattern.match(token).hasMatch()) {
        emit errorOccurred(QStringLiteral("Некорректный запрос входа из браузера."));
        return;
    }
    browserLoginToken_ = token;
    emit browserLoginRequestedChanged();
}

void AppController::approveBrowserLogin(const QString& masterPassword) {
    if (browserLoginToken_.isEmpty() || masterPassword.isEmpty()) {
        emit errorOccurred(QStringLiteral("Введите мастер-пароль для подтверждения входа."));
        return;
    }

#ifdef Q_OS_WIN
    const auto directory = browserLoginDirectory();
    QDir().mkpath(directory);
    QByteArray passwordBytes = masterPassword.toUtf8();
    DATA_BLOB input{static_cast<DWORD>(passwordBytes.size()),
                    reinterpret_cast<BYTE*>(passwordBytes.data())};
    DATA_BLOB encrypted{};
    const bool protectedOk = CryptProtectData(&input, L"lusakey browser login", nullptr, nullptr, nullptr,
                                               CRYPTPROTECT_UI_FORBIDDEN, &encrypted);
    SecureZeroMemory(passwordBytes.data(), passwordBytes.size());
    if (!protectedOk) {
        emit errorOccurred(QStringLiteral("Не удалось защитить подтверждение входа."));
        return;
    }

    QFile response(directory + QLatin1Char('/') + browserLoginToken_ + QStringLiteral(".response"));
    if (!response.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || response.write(reinterpret_cast<const char*>(encrypted.pbData), encrypted.cbData) != encrypted.cbData) {
        LocalFree(encrypted.pbData);
        emit errorOccurred(QStringLiteral("Не удалось отправить подтверждение в браузер."));
        return;
    }
    response.close();
    LocalFree(encrypted.pbData);
#else
    Q_UNUSED(masterPassword)
    emit errorOccurred(QStringLiteral("Подтверждение входа из браузера пока доступно только в Windows."));
    return;
#endif

    browserLoginToken_.clear();
    emit browserLoginRequestedChanged();
}

void AppController::denyBrowserLogin() {
    if (browserLoginToken_.isEmpty()) {
        return;
    }
    const auto directory = browserLoginDirectory();
    QDir().mkpath(directory);
    QFile denied(directory + QLatin1Char('/') + browserLoginToken_ + QStringLiteral(".denied"));
    if (denied.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        denied.write("denied");
    }
    browserLoginToken_.clear();
    emit browserLoginRequestedChanged();
}
