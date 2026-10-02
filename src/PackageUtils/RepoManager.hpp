/**
 * @file RepoManager.hpp
 * @brief Header for change configs in klass.conf.
 */

#ifndef REPOMANAGER_HPP
#define REPOMANAGER_HPP

#include <QStringList>

inline const auto OFFICIAL_HEADER = QStringLiteral("# Official Repo");
inline const auto THIRD_HEADER = QStringLiteral("# Third Mirrors");
inline const auto PRIORITY_HEADER = QStringLiteral("# Priorities");
inline const auto EXCEPTION_HEADER = QStringLiteral("# Exceptions");
inline const auto ADMIN_HEADER = QStringLiteral("# Administrative Actions");
inline const auto EMPTY = QStringLiteral("");

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
