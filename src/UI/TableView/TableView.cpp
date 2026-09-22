/**
 * @file TableView.cpp
 * @brief Implementation of the standardized custom TableView class.
 */

#include <QHeaderView>
#include <QPainter>

#include <TableView.hpp>

/**
 * @brief Constructs and initializes default table behaviors.
 * @param parent Optional parent widget.
 */
TableView::TableView(QWidget *parent) : QTableView(parent) {
    this->setAttribute(Qt::WA_StaticContents);
    this->setAttribute(Qt::WA_OpaquePaintEvent);
    this->setAttribute(Qt::WA_NoSystemBackground);

    this->horizontalHeader()->setStretchLastSection(true);
    this->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    this->verticalHeader()->setVerticalScrollMode(ScrollPerItem);
    this->verticalHeader()->setVisible(false);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setEditTriggers(NoEditTriggers);
    this->setSelectionBehavior(SelectRows);
    this->setSelectionMode(ExtendedSelection);

    this->setSortingEnabled(true);
    this->setCornerButtonEnabled(false);
    this->setShowGrid(false);
}

/**
 * @brief Sets whether horizontal grid lines should be drawn and triggers a viewport update.
 * @param show True to render horizontal lines, false to hide them.
 */
void TableView::setShowHorizontalGrid(const bool show) {
    if (showHorizontalGrid != show) {
        showHorizontalGrid = show;
        this->viewport()->update();
    }
}

/**
 * @brief Renders custom horizontal grid lines if the feature is active.
 * @param event The paint event.
 */
void TableView::paintEvent(QPaintEvent *event) {
    QTableView::paintEvent(event);

    if (!showHorizontalGrid || !model() || model()->rowCount() == 0)
        return;

    QPainter painter(viewport());

    QColor gridColor = palette().color(QPalette::Mid);
    gridColor.setAlpha(80);

    painter.setPen(gridColor);

    const int rows = model()->rowCount();
    const int tableWidth = viewport()->width();

    for (int row = 0; row < rows; ++row) {
        const int y = rowViewportPosition(row) + rowHeight(row);
        painter.drawLine(0, y, tableWidth, y);
    }
}

/**
 * @brief Temporarily freezes updates and signals to optimize bulk data operations.
 */
void TableView::block() {
    savedGridState = showHorizontalGrid;
    showHorizontalGrid = false;

    this->setAttribute(Qt::WA_Hover, false);
    this->setUpdatesEnabled(false);
    this->blockSignals(true);

    if (this->selectionModel())
        this->selectionModel()->blockSignals(true);
}

/**
 * @brief Restores normal table operations and triggers a complete visual refresh.
 */
void TableView::ublock() {
    showHorizontalGrid = savedGridState;

    this->setAttribute(Qt::WA_Hover, true);
    this->setUpdatesEnabled(true);
    this->blockSignals(false);

    if (this->selectionModel())
        this->selectionModel()->blockSignals(false);

    this->viewport()->update();
}
