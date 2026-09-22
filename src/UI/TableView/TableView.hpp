/**
 * @file TableView.hpp
 * @brief Header for custom TableView.
 */

#ifndef TABLEVIEW_HPP
#define TABLEVIEW_HPP

#include <QTableView>

/**
 * @class TableView
 * @brief Custom QTableView wrapper providing standardized configuration and styling.
 */
class TableView : public QTableView {
    Q_OBJECT

public:
    explicit TableView(QWidget *parent = nullptr);

    void setShowHorizontalGrid(bool show);

    void block();

    void ublock();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool showHorizontalGrid{false}, savedGridState{false};
};

#endif
