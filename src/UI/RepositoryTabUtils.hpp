/**
 * @file RepositoryTabUtils.hpp
 * @brief Static helpers and constants for the RepositoryTab.
 */

#ifndef REPOSITORYTABUTILS_HPP
#define REPOSITORYTABUTILS_HPP

#include <QStandardItem>
#include <QString>

#include <SlackwareDefines.hpp>
#include <StatusIcons.hpp>

/**
 * @namespace RepositoryTabUtils
 * @brief Utility functions and constants for managing repository tab items and hierarchy repositories.
 */
namespace RepositoryTabUtils {
    constexpr int PackageStatusRole = Qt::UserRole + 8;

    PkgStatus pkgStatus(const QStandardItem *item);

    inline bool isExcluded(const QStandardItem *item) { return pkgStatus(item) == PkgStatus::Excluded; }

    void setPackageStatus(QStandardItem *item, PkgStatus status);

    inline bool isHierarchyRepo(const QString &r) {
        return r.compare(SLACK_OFICIAL) == 0 || r.contains(SLACK_PATCHES) || r.contains(SLACK_TESTING);
    }
}

#endif
