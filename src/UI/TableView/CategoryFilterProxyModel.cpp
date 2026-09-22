/**
 * @file CategoryFilterProxyModel.cpp
 * @brief Implementation of the CategoryFilterProxyModel class for model filtering logic.
 */

#include <CategoryFilterProxyModel.hpp>

/**
 * @brief Constructs a CategoryFilterProxyModel instance.
 * @param parent Optional parent QObject for lifetime management.
 */
CategoryFilterProxyModel::CategoryFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {}

/**
 * @brief Sets the active category filter string.
 * @param cat Category name to filter by.
 */
void CategoryFilterProxyModel::setCategoryFilter(const QString &cat) {
    category = cat;
    invalidateFilter();
}

/**
 * @brief Sets the active free-text search term filter.
 * @param term Search term to match against name and description.
 */
void CategoryFilterProxyModel::setSearchFilter(const QString &term) {
    search = term;
    invalidateFilter();
}

/**
 * @brief Determines whether a row in the source model should be included in the proxy model.
 * @param sourceRow Row index in the source model.
 * @param sourceParent Parent index in the source model.
 * @return True if the row satisfies category and search criteria, false otherwise.
 */
bool CategoryFilterProxyModel::filterAcceptsRow(const int sourceRow, const QModelIndex &sourceParent) const {
    if (!category.isEmpty() && category != QStringLiteral("Overview")) {
        const QModelIndex catIndex = sourceModel()->index(sourceRow, 1, sourceParent);
        if (const QString rowCat = catIndex.data(Qt::UserRole).toString(); rowCat != category)
            return false;
    }

    if (!search.isEmpty()) {
        const QModelIndex nameIndex = sourceModel()->index(sourceRow, 1, sourceParent);
        const QModelIndex descIndex = sourceModel()->index(sourceRow, 3, sourceParent);
        const bool nameHit = nameIndex.data(Qt::DisplayRole).toString().contains(search, Qt::CaseInsensitive);
        const bool descHit = descIndex.data(Qt::DisplayRole).toString().contains(search, Qt::CaseInsensitive);
        if (!nameHit && !descHit)
            return false;
    }

    return true;
}
