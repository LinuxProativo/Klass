/**
 * @file VersionComboBoxDelegate.hpp
 * @brief header for customization column version.
 */

#ifndef VERSIONCOMBOBOXDELEGATE_HPP
#define VERSIONCOMBOBOXDELEGATE_HPP

#include <QStyledItemDelegate>

/**
 * @class VersionComboBoxDelegate
 * @brief A custom item delegate that renders a combo-box style dropdown inside view cells for version selection.
 */
class VersionComboBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit VersionComboBoxDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

signals:
    void versionChanged(const QModelIndex &index, int selectedVersionIndex) const;
};

#endif
