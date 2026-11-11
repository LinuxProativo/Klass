/**
 * @file NewConfigManager.cpp
 * @brief Implementation of filesystem scanning for .new configuration files.
 */

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <NewConfigManager.hpp>

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

QString NewConfigManager::readFileContent(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream in(&file);
    return in.readAll();
}
