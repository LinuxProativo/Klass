/**
 * @file LatencySortModel.hpp
 * @brief Header file for the LatencySortModel custom proxy model.
 */

#ifndef LATENCYSORTMODEL_HPP
#define LATENCYSORTMODEL_HPP

#include <QSortFilterProxyModel>

/**
 * @class LatencySortModel
 * @brief Proxy model that sorts the latency column numerically via Qt::UserRole.
 */
class LatencySortModel final : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit LatencySortModel(QObject *parent) : QSortFilterProxyModel(parent) {}

protected:
    [[nodiscard]] bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

#endif // LATENCYSORTMODEL_HPP
