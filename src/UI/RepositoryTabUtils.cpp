/**
 * @file RepositoryTabUtils.cpp
 * @brief Implementation of static helpers for the RepositoryTab.
 */

#include <RepositoryTabUtils.hpp>

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
}
