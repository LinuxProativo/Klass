/**
 * @file TreeWidget.hpp
 * @brief Header for customized QTreeView with custom branch rendering and clipboard support.
 */

#ifndef TREEWIDGET_HPP
#define TREEWIDGET_HPP

#include <QTreeView>

/**
 * @class TreeWidget
 * @brief Custom QTreeView with custom branch rendering, context menu, and text copying support.
 */
class TreeWidget : public QTreeView {
    Q_OBJECT

public:
    explicit TreeWidget(QWidget *parent = nullptr);

    void copySelectedItemsText() const;

protected:
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    void showContextMenu(const QPoint &pos);
};

#endif
