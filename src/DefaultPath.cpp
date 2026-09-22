/**
 * @file DefaultPath.cpp
 * @brief Provides logic to locate system and application files, handling path resolution for different environments.
 */

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <DefaultPath.hpp>

/**
 * @brief Initializes the path locator and configures directory string fallbacks.
 */
DefaultPath::DefaultPath(): oldDir(dir) {
    debug = new Debug::Debug();
    static const QRegularExpression regex("\\/(?:.(?!\\/))+$");
    oldDir = oldDir.remove(regex);
}

/**
 * @brief Resolves the absolute path for a given file or directory string.
 * @param str The file or directory name to be located.
 * @return The full path if found, otherwise an empty string.
 */
QString DefaultPath::findPath(const QString &str) {
    auto defDir = QString("%1/%2").arg((QFileInfo::exists(dir + str)) ? dir : oldDir, str);

    if (!QFileInfo::exists(defDir))
        return QString("%1/%2").arg((QFileInfo::exists(px + str)) ? px : newPx, str);

    return defDir;
}

/**
 * @brief Returns the directory path to be used if the specified target is found.
 * @param str Subdirectory and file location to serve as search base.
 * @return The validated path or an empty string if not found.
 */
QString DefaultPath::defaultPath(const QString &str) {
    if (auto defDir = findPath(str); QFileInfo::exists(defDir)) {
        debug->msg(QString("Directory for %1").arg(str), "DefaultPath", {defDir, Debug::Violet, Debug::Orange});
        return defDir;
    }

    debug->msg(QString("No directory defined for %1").arg(str), "DefaultPath", {Debug::LightRed});
    return {};
}
