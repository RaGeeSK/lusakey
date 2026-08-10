#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QMessageLogContext>

#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "bridge/activity_event_filter.h"
#include "bridge/app_controller.h"
#include "bridge/vault_list_model.h"
#include "bridge/totp_list_model.h"
#include "bridge/folder_list_model.h"

namespace {

constexpr auto kBrowserExtensionId = "iahmcbccpfkgbljjiggegnbfpnkbeipc";

void logHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    FILE* f = nullptr;
    fopen_s(&f, "C:\\Users\\MECHREVO\\AppData\\Local\\Temp\\lusakey_qt.log", "a");
    if (f) {
        fprintf(f, "[%s] %s (%s:%u)\n",
                type == QtDebugMsg ? "DEBUG" :
                type == QtWarningMsg ? "WARN" :
                type == QtCriticalMsg ? "CRITICAL" : "FATAL",
                msg.toLocal8Bit().constData(),
                ctx.file ? ctx.file : "", ctx.line);
        fclose(f);
    }
}

void loadBundledFonts() {
    const QDir fontsDir(QCoreApplication::applicationDirPath() + QStringLiteral("/fonts"));
    const QStringList fileNames = {
        QStringLiteral("Inter-Regular.ttf"),
        QStringLiteral("Inter-Medium.ttf"),
        QStringLiteral("Inter-SemiBold.ttf"),
        QStringLiteral("Inter-Bold.ttf"),
        QStringLiteral("JetBrainsMono-Regular.ttf"),
        QStringLiteral("JetBrainsMono-Medium.ttf"),
    };
    for (const auto& fileName : fileNames) {
        const auto path = fontsDir.filePath(fileName);
        if (QFontDatabase::addApplicationFont(path) < 0) {
            qWarning() << "lusakey: could not load bundled font (see resources/fonts/README.md):" << path;
        }
    }
}

QString exeDir() {
#ifdef Q_OS_WIN
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return QFileInfo(QString::fromWCharArray(buf)).absolutePath();
#else
    return QCoreApplication::applicationDirPath();
#endif
}

#ifdef Q_OS_WIN
void setNativeMessagingRegistryValue(const wchar_t* browserKey, const QString& manifestPath) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, browserKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        qWarning() << "lusakey: could not register native messaging host";
        return;
    }
    const auto path = manifestPath.toStdWString();
    RegSetValueExW(key, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(path.c_str()),
                   static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
}

void configureBrowserNativeHost() {
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QStringList candidates{
        appDir.filePath(QStringLiteral("../nmhost/lusakey-nmhost.exe")),
        appDir.filePath(QStringLiteral("lusakey-nmhost.exe")),
    };
    QString hostPath;
    for (const auto& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            hostPath = QFileInfo(candidate).absoluteFilePath();
            break;
        }
    }
    if (hostPath.isEmpty()) {
        return; // GUI-only build: nothing to register.
    }

    const auto localAppData = qEnvironmentVariable("LOCALAPPDATA");
    if (localAppData.isEmpty()) {
        qWarning() << "lusakey: LOCALAPPDATA is unavailable; browser integration was not configured";
        return;
    }
    const QDir manifestDir(localAppData + QStringLiteral("/lusakey/native-messaging"));
    if (!QDir().mkpath(manifestDir.absolutePath())) {
        qWarning() << "lusakey: could not create native-messaging directory";
        return;
    }

    const QJsonObject manifest{
        {QStringLiteral("name"), QStringLiteral("com.lusakey.nmhost")},
        {QStringLiteral("description"), QStringLiteral("lusakey browser integration")},
        {QStringLiteral("path"), QDir::toNativeSeparators(hostPath)},
        {QStringLiteral("type"), QStringLiteral("stdio")},
        {QStringLiteral("allowed_origins"), QJsonArray{QStringLiteral("chrome-extension://")
                                                         + QString::fromLatin1(kBrowserExtensionId) + QLatin1Char('/')}},
    };
    const auto manifestPath = manifestDir.filePath(QStringLiteral("com.lusakey.nmhost.json"));
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Compact)) < 0) {
        qWarning() << "lusakey: could not write native-messaging manifest";
        return;
    }
    manifestFile.close();

    setNativeMessagingRegistryValue(L"Software\\Google\\Chrome\\NativeMessagingHosts\\com.lusakey.nmhost", manifestPath);
    setNativeMessagingRegistryValue(L"Software\\Microsoft\\Edge\\NativeMessagingHosts\\com.lusakey.nmhost", manifestPath);
}
#else
void configureBrowserNativeHost() {}
#endif

QString browserLoginTokenFromArgs() {
    const auto args = QCoreApplication::arguments();
    const auto index = args.indexOf(QStringLiteral("--browser-login-token"));
    return index >= 0 && index + 1 < args.size() ? args.at(index + 1) : QString{};
}

} // namespace

int main(int argc, char* argv[]) {
#ifdef __linux__
    prctl(PR_SET_DUMPABLE, 0);
#endif

#ifdef Q_OS_WIN
    const QString appDir = exeDir();
    qputenv("QT_PLUGIN_PATH", (appDir + "/plugins").toLocal8Bit());
    qputenv("QML2_IMPORT_PATH", (appDir + "/../vcpkg_installed/x64-windows/Qt6/qml").toLocal8Bit());
#endif

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("lusakey"));
    QGuiApplication::setApplicationName(QStringLiteral("lusakey"));
    qInstallMessageHandler(logHandler);

    loadBundledFonts();
    configureBrowserNativeHost();

    AppController appController;

    auto* activityFilter = new ActivityEventFilter(&appController, &app);
    app.installEventFilter(activityFilter);

    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &appController,
                      [&appController](Qt::ApplicationState state) {
                          if (state == Qt::ApplicationSuspended || state == Qt::ApplicationHidden) {
                              appController.lock();
                          }
                      });

    QQmlApplicationEngine engine;
    engine.addImportPath(app.applicationDirPath() + QStringLiteral("/../vcpkg_installed/x64-windows/Qt6/qml"));
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &appController);
    engine.rootContext()->setContextProperty(QStringLiteral("vaultListModel"), appController.vaultListModel());
    engine.rootContext()->setContextProperty(QStringLiteral("totpListModel"), appController.totpListModel());
    engine.rootContext()->setContextProperty(QStringLiteral("folderListModel"), appController.folderListModel());

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QGuiApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("Lusakey", "Main");

    const auto browserLoginToken = browserLoginTokenFromArgs();
    if (!browserLoginToken.isEmpty()) {
        QMetaObject::invokeMethod(&appController, [browserLoginToken, &appController] {
            appController.beginBrowserLogin(browserLoginToken);
        }, Qt::QueuedConnection);
    }

    return app.exec();
}
