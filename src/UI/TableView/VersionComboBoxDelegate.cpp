/**
 * @file VersionComboBoxDelegate.cpp
 * @brief Implementation of a custom delegate for selecting versions within Qt view cells.
 */

#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

#include <VersionComboBoxDelegate.hpp>

/**
 * @brief Constructs a VersionComboBoxDelegate object.
 * @param parent The parent object in the Qt object hierarchy for memory management.
 */
VersionComboBoxDelegate::VersionComboBoxDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

/**
 * @brief Renders the visual appearance of the view item.
 * @param painter The QPainter object used to draw the UI elements.
 * @param option The current style and geometry options for the item being rendered.
 * @param index The model index identifying the specific item data.
 */
void VersionComboBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if (const QStringList versions = index.data(Qt::UserRole + 3).toStringList();
        versions.size() > 1) {
        painter->save();

        QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

        QStyleOptionComboBox boxOption;
        boxOption.rect = opt.rect;
        boxOption.state = opt.state | QStyle::State_Enabled;

        QRect arrowRect = style->subControlRect(QStyle::CC_ComboBox, &boxOption, QStyle::SC_ComboBoxArrow, opt.widget);
        boxOption.rect = arrowRect;
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &boxOption, painter, opt.widget);

        int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, opt.widget);
        QRect textRect = opt.rect.adjusted(3, 0, 0, 0);
        textRect.setRight(arrowRect.left() - margin);

        style->drawItemText(painter, textRect, Qt::AlignVCenter | Qt::AlignLeft, opt.palette,
                            opt.state.testFlag(QStyle::State_Enabled), opt.text, QPalette::Text);
        painter->restore();
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

/**
 * @brief Handles user interaction events on the item to trigger version switching.
 * @param event The Qt event received by the delegate (e.g., mouse click).
 * @param model The pointer to the data model associated with the view.
 * @param option The style and geometry configuration of the interacted item.
 * @param index The model index of the item that received the event.
 * @return True if the event was completely handled and should not propagate further; false otherwise.
 */
bool VersionComboBoxDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                                          const QModelIndex &index) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (const auto *mouseEvent = dynamic_cast<QMouseEvent *>(event); mouseEvent->button() == Qt::LeftButton) {
            if (const QStringList versions = index.data(Qt::UserRole + 3).toStringList(); versions.size() > 1) {
                const QString currentVersion = index.data(Qt::DisplayRole).toString();

                QMenu menu;
                for (int i = 0; i < versions.size(); ++i) {
                    QAction *action = menu.addAction(versions.at(i));
                    action->setData(i);

                    if (versions.at(i) == currentVersion) {
                        action->setCheckable(true);
                        action->setChecked(true);
                    }
                }

                const QWidget *pw = const_cast<QWidget *>(option.widget);
                const QPoint globalPos = pw ? pw->mapToGlobal(option.rect.bottomLeft()) : QCursor::pos();
                const QAction *selectedAction = menu.exec(globalPos);

                if (selectedAction && selectedAction->text() != currentVersion) {
                    const int selectedIdx = selectedAction->data().toInt();
                    model->setData(index, selectedAction->text(), Qt::DisplayRole);
                    emit versionChanged(index, selectedIdx);
                }
                return true;
            }
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
