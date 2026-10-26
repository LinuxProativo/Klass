/**
 * @file CategoryFilterProxyModel.hpp
 * @brief Header for filtering and searching packages using CategoryFilterProxyModel.
 */

#ifndef CATEGORYFILTERPROXYMODEL_HPP
#define CATEGORYFILTERPROXYMODEL_HPP

#include <QSortFilterProxyModel>
#include <QString>

/**
 * @class CategoryFilterProxyModel
 * @brief Filters rows by category and, optionally, by a free-text search term.
 */
class CategoryFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit CategoryFilterProxyModel(QObject *parent = nullptr);

    void setCategoryFilter(const QString &cat);

    void setSearchFilter(const QString &term);

    [[nodiscard]] QString searchFilter() const { return search; }

protected:
    [[nodiscard]] bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString category;
    QString search;
};

#endif
