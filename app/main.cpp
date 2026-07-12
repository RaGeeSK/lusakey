#include <QCoreApplication>
#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "bridge/activity_event_filter.h"
#include "bridge/app_controller.h"
// Needed for the complete VaultListModel (QObject-derived) type: without
// it, app_controller.h's forward declaration leaves setContextProperty()
// unable to see it's a QObject*, so overload resolution falls through to
// the QVariant overload and fails with a deleted-constructor error.
#include "bridge/vault_list_model.h"
#include "bridge/totp_list_model.h"

namespace {

// Fonts are loaded from a plain filesystem path next to the executable
// (resolved from applicationDirPath(), NOT the current working directory —
// a bare relative path would break depending on how the exe is launched,
// e.g. a desktop shortcut with a different working directory), NOT embedded
// via the Qt resource system — see resources/fonts/README.md and
// app/CMakeLists.txt's post-build copy step for how the files get there.
// The actual .ttf files (Inter, JetBrains Mono; both OFL). A missing file
// just skips with a warning (QFontDatabase::addApplicationFont returns -1)
// rather than failing.
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

} // namespace

int main(int argc, char* argv[]) {
#ifdef __linux__
    // Best-effort: suppress core dumps of this process, since a core dump
    // would contain decrypted vault secrets still resident in memory.
    // Silently a no-op on kernels/configurations where it's disallowed —
    // there's nothing actionable to surface to the user if it fails.
    prctl(PR_SET_DUMPABLE, 0);
#endif

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("lusakey"));
    QGuiApplication::setApplicationName(QStringLiteral("lusakey"));

    loadBundledFonts();

    AppController appController;

    // App-wide auto-lock: reset the idle timer on any input activity
    // anywhere in the window (see ActivityEventFilter's docs for what this
    // does and doesn't cover), and lock immediately if the OS suspends the
    // app or the window is hidden (covers sleep/minimize on most platforms).
    auto* activityFilter = new ActivityEventFilter(&appController, &app);
    app.installEventFilter(activityFilter);

    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &appController,
                      [&appController](Qt::ApplicationState state) {
                          if (state == Qt::ApplicationSuspended || state == Qt::ApplicationHidden) {
                              appController.lock();
                          }
                      });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &appController);
    engine.rootContext()->setContextProperty(QStringLiteral("vaultListModel"), appController.vaultListModel());
    engine.rootContext()->setContextProperty(QStringLiteral("totpListModel"), appController.totpListModel());

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QGuiApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("Lusakey", "Main");

    return app.exec();
}
