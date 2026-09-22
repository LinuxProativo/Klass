/**
 * @file RepoManager.hpp
 * @brief Header for change configs in klass.conf.
 */

#ifndef REPOMANAGER_HPP
#define REPOMANAGER_HPP

#include <QStringList>

/**
 * @class RepoManager
 * @brief Manages the configuration file for software repositories.
 */
class RepoManager {
public:
    static QStringList readConf();

    static bool writeConf(const QStringList &lines);

    static QStringList fetchChangeLogs();

private:
    static bool writeLines(const QString &path, const QStringList &lines);
};

#endif
