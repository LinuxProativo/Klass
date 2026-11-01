/**
 * @file RepoUtils.hpp
 * @brief Namespace containing utility functions for package and repository management.
 */

#ifndef REPOUTILS_HPP
#define REPOUTILS_HPP

#include <QStringList>
#include <QStringView>

inline const auto OFFICIAL_HEADER = QStringLiteral("# Official Repo");
inline const auto THIRD_HEADER = QStringLiteral("# Third Mirrors");
inline const auto VERIFICATION_HEADER = QStringLiteral("# Verifications");
inline const auto PRIORITY_HEADER = QStringLiteral("# Priorities");
inline const auto EXCEPTION_HEADER = QStringLiteral("# Exceptions");
inline const auto ADMIN_HEADER = QStringLiteral("# Administrative Actions");
inline const auto EMPTY = QStringLiteral("");

/**
 * @namespace RepoUtils
 * @brief Utilities for Manages the configuration file for software repositories and others functions.
 */
namespace RepoUtils {
    QString normalizeSpaces(QStringView raw);

    QStringList readConf();

    QStringList fetchChangeLogs();

    bool writeConf(const QStringList &lines);

    bool writeLines(const QString &path, const QStringList &lines);
}

#endif
