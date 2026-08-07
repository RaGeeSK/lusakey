#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
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

    return app.exec();
}
