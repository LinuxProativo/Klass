/**
 * @file NewConfigManager.cpp
 * @brief Implementation of filesystem scanning and file reading for .new configurations.
 */

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <NewConfigManager.hpp>

/**
 * @brief Scans a root directory recursively to find all files with a .new extension.
 * @param rootDir The root directory where searching begins (defaults to /etc).
 * @return A list of NewConfigFileItem structs representing each found .new file.
 */
QList<NewConfigFileItem> NewConfigManager::scanNewFiles(const QString &rootDir) {
    QList<NewConfigFileItem> results;

    if (!QDir(rootDir).exists())
        return results;

    QDirIterator it(rootDir, QStringList{QStringLiteral("*.new")}, QDir::Files,
                    QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

    while (it.hasNext()) {
        it.next();
        const QString newPath = it.filePath();
        const QString origPath = newPath.chopped(4);

        NewConfigFileItem item;
        item.newFilePath = newPath;
        item.originalFilePath = origPath;
        item.originalExists = QFile::exists(origPath);
        item.action = NewConfigAction::Keep;

        results.append(std::move(item));
    }

    return results;
}

/**
 * @brief Reads the entire plain text content of a file from disk.
 * @param filePath Absolute path of the file to be read.
 * @return The content of the file as a QString, or an empty string if reading fails.
 */
QString NewConfigManager::readFileContent(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream in(&file);
    return in.readAll();
}
