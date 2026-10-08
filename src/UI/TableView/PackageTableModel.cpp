/**
 * @file PackageTableModel.cpp
 * @brief Implementation of custom lightweight table model.
 */

#include <PackageTableModel.hpp>
#include <RepositoryTabUtils.hpp>

/**
 * @brief Constructs a PackageTableModel instance.
 * @param parent Optional parent QObject.
 */
PackageTableModel::PackageTableModel(QObject *parent) : QAbstractTableModel(parent) {}

/**
 * @brief Returns the total number of package rows in the model.
 * @param parent The parent model index (unused for flat table models).
 * @return The total number of rows, or 0 if parent is valid.
 */
int PackageTableModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

/**
 * @brief Returns the 4 fixed columns: Status, Name, Version, Description.
 * @param parent The parent model index (unused for flat table models).
 * @return The total number of columns, or 0 if parent is valid.
 */
int PackageTableModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : 4;
}

/**
 * @brief Supplies horizontal column headers.
 * @param section The column index (0 to 3) requesting data.
 * @param orientation The orientation of the header (strictly Qt::Horizontal here).
 * @param role The display role requested by the view (strictly Qt::DisplayRole here).
 * @return The header text wrapped in a QVariant, or an empty QVariant if invalid.
 */
QVariant PackageTableModel::headerData(const int section, const Qt::Orientation orientation, const int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Status");
            case 1: return tr("Name");
            case 2: return tr("Version");
            case 3: return tr("Description");
            default: break;
        }
    }
    return {};
}

/**
 * @brief Resolves role data dynamically on demand without storing duplicate QVariants in memory.
 * @param index The specific model index (row and column) being queried.
 * @param role The purpose of the data requested (Display, Decoration, or Custom UserRoles).
 * @return The underlying data appropriate for the requested role wrapped in a QVariant.
 */
QVariant PackageTableModel::data(const QModelIndex &index, const int role) const {
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};

    const auto &row = m_rows.at(index.row());

    if (const int col = index.column(); col == 0) {
        if (role == Qt::DecorationRole)
            return StatusIcons::statusIcon(row.status);
        if (role == RepositoryTabUtils::PackageStatusRole)
            return static_cast<int>(row.status);
        if (role == Qt::UserRole + 3)
            return row.isPrioritized;
        if (role == Qt::UserRole + 5)
            return row.actionSymbol;
        if (role == Qt::UserRole + 6)
            return row.targetVersion;
        if (role == Qt::UserRole + 7)
            return row.targetRepo;
        if (role == Qt::DisplayRole)
            return QString{};
    } else if (col == 1) {
        if (role == Qt::DisplayRole)
            return row.name;
        if (role == Qt::UserRole)
            return row.category;
        if (role == Qt::UserRole + 1)
            return row.isInstalled;
        if (role == Qt::UserRole + 2)
            return row.repoName;
    } else if (col == 2) {
        if (role == Qt::DisplayRole)
            return row.version;
        if (role == Qt::UserRole + 3)
            return row.allVersions;
        if (role == Qt::UserRole + 4)
            return QVariant::fromValue(row.pkgList);
    } else if (col == 3) {
        if (role == Qt::DisplayRole)
            return row.description;
    }

    return {};
}

/**
 * @brief Replaces all rows at once using model reset semantics.
 * @param rows The new list of package data items to populate the model.
 */
void PackageTableModel::setRows(QList<PackageRowData> rows) {
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}

/**
 * @brief Notifies the view that data across a row has changed.
 * @param row The zero-based index of the updated data row.
 */
void PackageTableModel::notifyRowChanged(const int row) {
    if (row >= 0 && row < m_rows.size()) {
        const QModelIndex left = index(row, 0);
        const QModelIndex right = index(row, 3);
        emit dataChanged(left, right);
    }
}

/**
 * @brief Clears all model rows.
 */
void PackageTableModel::clear() {
    beginResetModel();
    m_rows.clear();
    endResetModel();
}
