/**
 * @file LatencySortModel.cpp
 * @brief Implementation of the LatencySortModel proxy class.
 */

#include <LatencySortModel.hpp>

/**
 * @brief Compares two model index items for sorting.
 * @param left Left model index to compare.
 * @param right Right model index to compare.
 * @return True if left value is strictly less than right value; false otherwise.
 */
bool LatencySortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    if (left.column() == 2)
        return left.data(Qt::UserRole).toLongLong() < right.data(Qt::UserRole).toLongLong();

    return QSortFilterProxyModel::lessThan(left, right);
}
