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

namespace {

// Fonts are loaded from a plain filesystem path next to the executable,
// NOT embedded via the Qt resource system — see resources/fonts/README.md:
// the actual .ttf files (Inter, JetBrains Mono; both OFL) must be downloaded
// separately and placed there, since this project's tooling can't embed
// binary font files. A missing file just skips with a warning
// (QFontDatabase::addApplicationFont returns -1) rather than failing.
void loadBundledFonts() {
    const QStringList candidates = {
        QStringLiteral("fonts/Inter-Regular.ttf"),
        QStringLiteral("fonts/Inter-Medium.ttf"),
        QStringLiteral("fonts/Inter-SemiBold.ttf"),
        QStringLiteral("fonts/Inter-Bold.ttf"),
        QStringLiteral("fonts/JetBrainsMono-Regular.ttf"),
        QStringLiteral("fonts/JetBrainsMono-Medium.ttf"),
    };
    for (const auto& path : candidates) {
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

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QGuiApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("Lusakey", "Main");

    return app.exec();
}
