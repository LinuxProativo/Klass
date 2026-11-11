/**
 * @file NewConfigManager.hpp
 * @brief Utilities for finding and reading Slackware .new configuration files.
 */

#ifndef NEWCONFIGMANAGER_HPP
#define NEWCONFIGMANAGER_HPP

#include <QList>
#include <QString>

/**
 * @enum NewConfigAction
 * @brief Action to apply to a .new configuration file.
 */
enum class NewConfigAction {
    Keep,
    Overwrite,
    Remove
};

/**
 * @struct NewConfigFileItem
 * @brief Metadata for a discovered .new file and its original counterpart.
 */
struct NewConfigFileItem {
    QString newFilePath{};
    QString originalFilePath{};
    bool originalExists{false};
    NewConfigAction action{NewConfigAction::Keep};
};

/**
 * @class NewConfigManager
 * @brief Scanner and processor for Slackware system configuration updates.
 */
class NewConfigManager {
public:
    static QList<NewConfigFileItem> scanNewFiles(const QString &rootDir = QStringLiteral("/etc"));

    static QString readFileContent(const QString &filePath);
};

#endif
