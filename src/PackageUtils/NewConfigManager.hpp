/**
 * @file NewConfigManager.hpp
 * @brief Header for scanning and reading Slackware .new configuration files.
 */

#ifndef NEWCONFIGMANAGER_HPP
#define NEWCONFIGMANAGER_HPP

#include <QList>
#include <QString>

/**
 * @enum NewConfigAction
 * @brief Action to be executed on a discovered .new configuration file.
 */
enum class NewConfigAction {
    Keep,
    Overwrite,
    Remove
};

/**
 * @struct NewConfigFileItem
 * @brief Structure containing metadata and targeted action for a .new file.
 */
struct NewConfigFileItem {
    QString newFilePath{};
    QString originalFilePath{};
    bool originalExists{false};
    NewConfigAction action{NewConfigAction::Keep};
};

/**
 * @class NewConfigManager
 * @brief Filesystem inspection utility for Slackware system configuration updates.
 */
class NewConfigManager {
public:
    static QList<NewConfigFileItem> scanNewFiles(const QString &rootDir = QStringLiteral("/etc"));

    static QString readFileContent(const QString &filePath);
};

#endif
