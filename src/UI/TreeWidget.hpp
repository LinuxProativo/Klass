/**
 * @file TreeWidget.hpp
 * @brief Header for TreeWidget.
 */

#ifndef TREEWIDGET_HPP
#define TREEWIDGET_HPP

#include <QTreeWidget>

/**
 * @class TreeWidget
 * @brief Custom QTreeWidget with custom branch rendering, context menu, and text copying support.
 */
class TreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit TreeWidget(QWidget *parent = nullptr);

protected:
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    void showContextMenu(const QPoint &pos);

    void copySelectedItemsText() const;
};

#endif
