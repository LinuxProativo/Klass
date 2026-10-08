/**
 * @file PackageTableModel.hpp
 * @brief Header for custom lightweight table model for package management.
 */

#ifndef PACKAGETABLEMODEL_HPP
#define PACKAGETABLEMODEL_HPP

#include <QAbstractTableModel>
#include <QList>
#include <QStringList>

#include <Packages.hpp>
#include <SlackwareDefines.hpp>
#include <StatusIcons.hpp>

/**
 * @struct PackageRowData
 * @brief Memory-efficient contiguous representation of a package table row.
 */
struct PackageRowData {
    PkgStatus status{PkgStatus::Available};
    bool isPrioritized{false};
    QString name{};
    QString version{};
    QString description{};
    QString category{};
    QString repoName{};
    bool isInstalled{false};
    QString actionSymbol{};
    QString targetVersion{};
    QString targetRepo{};
    QStringList allVersions{};
    QList<PkgInfo> pkgList{};
};

/**
 * @class PackageTableModel
 * @brief Custom table model replacing QStandardItemModel for drastic RAM reduction.
 */
class PackageTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit PackageTableModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setRows(QList<PackageRowData> rows);

    [[nodiscard]] PackageRowData &rowAt(const int row) { return m_rows[row]; }

    [[nodiscard]] const PackageRowData &rowAt(const int row) const { return m_rows.at(row); }

    void notifyRowChanged(int row);

    void clear();

private:
    QList<PackageRowData> m_rows{};
};

#endif
