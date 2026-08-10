#include "font_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFontDatabase>
#include <QDebug>

bool FontManager::loadBundledFonts() {
    const QDir fontsDir(QCoreApplication::applicationDirPath() + QStringLiteral("/fonts"));
    loadedFamilies_.clear();
    failedFamilies_.clear();

    for (const auto& fileName : expectedFonts_) {
        const auto path = fontsDir.filePath(fileName);
        const auto fontId = loadFont(path);
        if (fontId < 0) {
            qWarning() << "lusakey: could not load bundled font:" << path;
            failedFamilies_.append(fileName);
        } else {
            // QFontDatabase::addApplicationFont returns the font ID on success.
            // We need to get the actual font family name that was registered.
            const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                loadedFamilies_.append(families.first());
            }
        }
    }

    return failedFamilies_.isEmpty();
}

bool FontManager::isFontAvailable(const QString& fontFamily) const {
    const QFontDatabase db;
    return db.families().contains(fontFamily);
}

QStringList FontManager::loadedFontFamilies() const {
    return loadedFamilies_;
}

QStringList FontManager::failedFontFamilies() const {
    return failedFamilies_;
}

int FontManager::loadFont(const QString& path) {
    return QFontDatabase::addApplicationFont(path);
}
