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
 * @brief Utilities for managing configuration files, package strings, and filesystem operations.
 */
namespace RepoUtils {
    QString detectArch();

    QString normalizeSpaces(QStringView raw);

    bool parseBool(QStringView value, bool defaultValue = false);

    bool isPackageArchive(QStringView fileName);

    inline QStringView stripPackageExtension(const QStringView fileName){
        return isPackageArchive(fileName) ? fileName.chopped(4) : fileName;
    }

    void appendPackageDescription(QStringView rawLine, QStringView prefix,
                                  QString &outShortDesc, QString &outDetailedDesc);

    QStringList readConf();

    QStringList fetchChangeLogs();

    bool writeConf(const QStringList &lines);

    bool writeLines(const QString &path, const QStringList &lines);

    QString readConfigValue(const QString &keyPrefix);

    bool setConfigValue(const QString &sectionHeader, const QString &keyPrefix, const QString &newLine);

    bool removeConfigValue(const QString &keyPrefix);

    bool removeConfigLines(const QSet<QString> &targets);

    bool appendConfigLine(const QString &sectionHeader, const QString &newLine);

    bool replaceConfigLine(const QString &oldLine, const QString &newLine);

    bool replaceConfigSection(const QString &sectionHeader, const QStringList &newSectionLines);
}

#endif
