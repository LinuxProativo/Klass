/**
 * @file TreeWidget.cpp
 * @brief Implementation of TreeWidget adapted to QTreeView with custom rendering and clipboard support.
 */

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QPen>

#include <TreeWidget.hpp>

/**
 * @brief Constructs a TreeWidget instance based on QTreeView.
 * @param parent The parent widget, or nullptr if none.
 */
TreeWidget::TreeWidget(QWidget *parent) : QTreeView(parent) {
    this->setAttribute(Qt::WA_StaticContents);
    this->setAttribute(Qt::WA_OpaquePaintEvent);
    this->setAttribute(Qt::WA_NoSystemBackground);
    this->setRootIsDecorated(true);
    this->setUniformRowHeights(true);
    this->setSelectionMode(ExtendedSelection);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->header()->setStretchLastSection(true);

    connect(this, &QTreeView::customContextMenuRequested, this, &TreeWidget::showContextMenu);
}

/**
 * @brief Displays the context menu at the requested position.
 * @param pos The local position where the menu should appear.
 */
void TreeWidget::showContextMenu(const QPoint &pos) {
    if (!this->selectionModel() || !this->selectionModel()->hasSelection())
        return;

    QMenu contextMenu(this);
    QAction *copyAction = contextMenu.addAction(tr("Copy"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &TreeWidget::copySelectedItemsText);
    contextMenu.exec(this->viewport()->mapToGlobal(pos));
}

/**
 * @brief Copies the selected items text to the system clipboard.
 */
void TreeWidget::copySelectedItemsText() const {
    if (!this->selectionModel())
        return;

    const QModelIndexList selectedRows = this->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return;

    QStringList textList;
    textList.reserve(selectedRows.size());

    for (const QModelIndex &index: selectedRows)
        textList.append(index.data(Qt::DisplayRole).toString());

    QApplication::clipboard()->setText(textList.join(u'\n'));
}

/**
 * @brief Draws custom branch lines for hierarchical tree items.
 * @param painter The painter used for rendering the branches.
 * @param rect The bounding rectangle for drawing the branches.
 * @param index The model index of the item being drawn.
 */
void TreeWidget::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const {
    const bool isRoot = !index.parent().isValid();
    const bool hasChildren = this->model() && this->model()->hasChildren(index);

    if (isRoot && hasChildren) {
        QTreeView::drawBranches(painter, rect, index);
        return;
    }

    if (!isRoot) {
        const int indent = this->indentation();
        QStyleOptionViewItem opt;
        opt.initFrom(this);

        if (index.parent().parent().isValid())
            opt.rect = QRect(rect.left() + rect.width() - indent - 10, rect.top(), indent, rect.height());
        else opt.rect = rect;
        opt.state = QStyle::State_Item;

        if (hasChildren)
            opt.state |= QStyle::State_Children;
        if (this->isExpanded(index))
            opt.state |= QStyle::State_Open;

        this->style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, this);
    }

    painter->save();
    painter->setPen(QPen(this->palette().color(QPalette::Mid), 1, Qt::SolidLine));

    const int indent = this->indentation();
    const int centerX = rect.left() + rect.width() - indent + 8;
    const int centerY = rect.top() + rect.height() / 2;
    const bool hasNextSibling = index.sibling(index.row() + 1, 0).isValid();

    if (hasChildren) {
        painter->drawLine(centerX, centerY, rect.right(), centerY);
        painter->drawLine(centerX, rect.top(), centerX, centerY);
        if (hasNextSibling)
            painter->drawLine(centerX, centerY, centerX, rect.bottom());
    } else {
        painter->drawLine(centerX, centerY, rect.right(), centerY);
        if (hasNextSibling) {
            painter->drawLine(centerX, rect.top(), centerX, rect.bottom());
        } else {
            painter->drawLine(centerX, rect.top(), centerX, centerY);
        }
    }

    QModelIndex ancestor = index.parent();
    int depth = 1;

    while (ancestor.isValid() && ancestor.parent().isValid()) {
        const int ancestorX = centerX - depth * indent;
        if (ancestor.sibling(ancestor.row() + 1, 0).isValid())
            painter->drawLine(ancestorX, rect.top(), ancestorX, rect.bottom());
        ancestor = ancestor.parent();
        depth++;
    }

    painter->restore();
}

/**
 * @brief Handles key press events to support copying selected items.
 * @param event Event for pressed key.
 */
void TreeWidget::keyPressEvent(QKeyEvent *event) {
    if (event->matches(QKeySequence::Copy)) {
        copySelectedItemsText();
        event->accept();
        return;
    }

    QTreeView::keyPressEvent(event);
}
