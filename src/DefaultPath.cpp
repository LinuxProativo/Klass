/**
 * @file DefaultPath.cpp
 * @brief Provides logic to locate system and application files, handling path resolution for different environments.
 */

#include <QCoreApplication>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>

#include <Debug.hpp>
#include <DefaultPath.hpp>

namespace {
    struct PathContext {
        QString dir;
        QString oldDir;
        QString px{"/usr/local/share/klass"};
        QString newPx{"/usr/share/klass"};
        QHash<QString, QString> cache;

        PathContext() {
            dir = QCoreApplication::applicationDirPath();
            oldDir = dir;
            static const QRegularExpression regex(R"(\/(?:.(?!\/))+$)");
            oldDir.remove(regex);
        }
    };

    PathContext &context() {
        static PathContext ctx;
        return ctx;
    }
}

/**
 * @brief Resolves the absolute path for a given file or directory string.
 * @param str The file or directory name to be located.
 * @return The full path if found, otherwise an empty string.
 */
QString DefaultPath::findPath(const QString &str) {
    auto &ctx = context();
    auto defDir = QString("%1/%2").arg((QFileInfo::exists(ctx.dir + str)) ? ctx.dir : ctx.oldDir, str);

    if (!QFileInfo::exists(defDir))
        return QString("%1/%2").arg((QFileInfo::exists(ctx.px + str)) ? ctx.px : ctx.newPx, str);

    return defDir;
}

/**
 * @brief Returns the directory path to be used if the specified target is found.
 * @param str Subdirectory and file location to serve as search base.
 * @return The validated path or an empty string if not found.
 */
QString DefaultPath::defaultPath(const QString &str) {
    auto &ctx = context();
    if (const auto it = ctx.cache.find(str); it != ctx.cache.end())
        return it.value();

    if (auto defDir = findPath(str); QFileInfo::exists(defDir)) {
        Debug::msg(QString("Directory for %1").arg(str), "DefaultPath", {defDir, DColor::Violet, DColor::Orange});
        ctx.cache.insert(str, defDir);
        return defDir;
    }

    Debug::msg(QString("No directory defined for %1").arg(str), "DefaultPath", {DColor::LightRed});
    return {};
}
