/**
 * @file CenterDelegate.hpp
 * @brief header for delegate class.
 */

#ifndef CENTERDELEGATE_HPP
#define CENTERDELEGATE_HPP

#include <QStyledItemDelegate>

/**
 * @class CenterDelegate
 * @brief A custom item delegate to center-align rendering and dynamically style status glyphs inside view cells.
 */
class CenterDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit CenterDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif
