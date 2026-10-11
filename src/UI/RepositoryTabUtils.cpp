/**
 * @file RepositoryTabUtils.cpp
 * @brief Implementation of static helpers for the RepositoryTab.
 */

#include <RepositoryTabUtils.hpp>
#include <Utils.hpp>

namespace RepositoryTabUtils {
    /**
     * @brief Retrieves the package status stored within a model item's custom data role.
     * @param item Pointer to the QStandardItem to inspect.
     * @return Extracted PkgStatus enum value, or PkgStatus::Available if item is null.
     */
    PkgStatus pkgStatus(const QStandardItem *item) {
        if (!item)
            return PkgStatus::Available;
        return static_cast<PkgStatus>(item->data(PackageStatusRole).toInt());
    }

    /**
     * @brief Updates the package status data role for a specific model item.
     * @param item Pointer to the target QStandardItem to update.
     * @param status The new PkgStatus value to assign.
     */
    void setPackageStatus(QStandardItem *item, PkgStatus status) {
        if (!item)
            return;

        if (const QVariant currentStatus = item->data(PackageStatusRole);
            currentStatus.isValid() && pkgStatus(item) == status)
            return;

        item->setData(static_cast<int>(status), PackageStatusRole);
        item->setIcon(StatusIcons::statusIcon(status));
        item->setText(QString{});
    }

    /**
     * @brief Evaluates repository precedence priority based on testing status and attachment rules.
     * @param repo Repository identifier to test.
     * @param attachTesting Whether testing repository is attached to official repository.
     * @param rowIsTestingOnly Whether all row options belong exclusively to testing.
     * @return Integer priority rating (higher is prioritized).
     */
    int repoPriority(const QString &repo, const bool attachTesting, const bool rowIsTestingOnly) {
        if ((attachTesting || rowIsTestingOnly) && repo.contains(SLACK_TESTING))
            return 3;
        if (repo.contains(SLACK_PATCHES))
            return 2;
        if (repo.compare(SLACK_OFICIAL) == 0)
            return 1;
        return 0;
    }

    /**
     * @brief Determines whether an upgrade or replacement is available for a package list.
     * @param pkgList Candidate package list for a given row.
     * @param attachTesting Whether testing repository is attached to official repository.
     * @return True if a newer version or higher-priority package is available, false otherwise.
     */
    bool computeHasUpdate(const QList<PkgInfo> &pkgList, const bool attachTesting) {
        if (pkgList.size() <= 1)
            return false;

        bool rowIsTestingOnly = true;
        for (const auto &p: pkgList) {
            if (!p.repoName.contains(SLACK_TESTING)) {
                rowIsTestingOnly = false;
                break;
            }
        }

        const PkgInfo *installedCandidate = nullptr;
        for (const auto &p: pkgList) {
            if (p.isInstalled) {
                installedCandidate = &p;
                break;
            }
        }

        const PkgInfo *bestAvailable = nullptr;
        int maxPriority = -1;

        for (const auto &p: pkgList) {
            if (p.isInstalled)
                continue;

            if (p.repoName.contains(SLACK_TESTING) && !attachTesting && !rowIsTestingOnly)
                continue;

            if (const int priority = repoPriority(p.repoName, attachTesting, rowIsTestingOnly);
                priority > maxPriority) {
                maxPriority = priority;
                bestAvailable = &p;
            } else if (priority == maxPriority && bestAvailable) {
                if (Utils::isVersionNewer(p.version, bestAvailable->version))
                    bestAvailable = &p;
            }
        }

        if (installedCandidate && bestAvailable) {
            const int installedPriority = repoPriority(installedCandidate->repoName, attachTesting, rowIsTestingOnly);
            if (maxPriority > installedPriority)
                return true;

            if (maxPriority == installedPriority) {
                if (bestAvailable->version != installedCandidate->version &&
                    Utils::isVersionNewer(bestAvailable->version, installedCandidate->version)) {
                    return true;
                }
            }
        }

        return false;
    }
}
