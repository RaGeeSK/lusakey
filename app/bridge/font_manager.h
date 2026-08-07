#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include <array>
#include <string>

// Centralized font management for lusakey.
//
// Loads fonts from a plain filesystem path next to the executable
// (resolved from applicationDirPath(), NOT the current working directory),
// NOT embedded via the Qt resource system — see resources/fonts/README.md
// and app/CMakeLists.txt's post-build copy step for how the files get there.
//
// The actual .ttf files (Inter, JetBrains Mono; both OFL). A missing file
// just skips with a warning (QFontDatabase::addApplicationFont returns -1)
// rather than failing.
//
// This is a singleton, accessible via FontManager::instance().
// It is intentionally Qt-dependent (QFontDatabase) and lives in app/bridge/,
// not libs/core/ (which has zero Qt dependencies per AGENTS.md).
class FontManager : public QObject {
    Q_OBJECT

public:
    // Returns the singleton instance. Creates it on first call.
    static FontManager& instance();

    // Loads all bundled fonts from the fonts/ directory next to the executable.
    // Returns true if all expected fonts were loaded successfully, false if
    // any font failed to load (but the application continues regardless).
    bool loadBundledFonts();

    // Checks if a specific font family is available (loaded or system).
    bool isFontAvailable(const QString& fontFamily) const;

    // Returns the list of font families that were successfully loaded.
    QStringList loadedFontFamilies() const;

    // Returns the list of font families that were expected but failed to load.
    QStringList failedFontFamilies() const;

private:
    // Private constructor for singleton pattern.
    FontManager() = default;
    ~FontManager() = default;

    // Prevent copying.
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    // Loads a single font file and returns the font ID (or -1 on failure).
    int loadFont(const QString& path);

    std::array<QString, 6> expectedFonts_ = {{
        QStringLiteral("Inter-Regular.ttf"),
        QStringLiteral("Inter-Medium.ttf"),
        QStringLiteral("Inter-SemiBold.ttf"),
        QStringLiteral("Inter-Bold.ttf"),
        QStringLiteral("JetBrainsMono-Regular.ttf"),
        QStringLiteral("JetBrainsMono-Medium.ttf"),
    }};

    QStringList loadedFamilies_;
    QStringList failedFamilies_;
};
