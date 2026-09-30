/**
 * @file VersionComboBoxDelegate.cpp
 * @brief Implementation of a custom delegate for selecting versions within Qt view cells.
 */

#include <QAbstractItemView>
#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QVersionNumber>

#include <DefaultPath.hpp>
#include <Packages.hpp>
#include <SlackwareDefines.hpp>
#include <VersionComboBoxDelegate.hpp>

inline constexpr int margin = 5;
inline constexpr int height = 6;
inline constexpr int iconSize = 16;

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

        const int buttonSize = qMax(height, opt.rect.height() - margin * 2);

        const QRect buttonRect(
            opt.rect.x() + opt.rect.width() - margin - buttonSize,
            opt.rect.y() + margin,
            buttonSize, buttonSize
        );

        QStyleOptionButton buttonOpt;
        buttonOpt.rect = buttonRect;
        buttonOpt.state = QStyle::State_Enabled | QStyle::State_Raised;
        style->drawPrimitive(QStyle::PE_PanelButtonCommand, &buttonOpt, painter, opt.widget);

        QStyleOptionComboBox boxOption;
        boxOption.rect = buttonRect;
        boxOption.state = opt.state | QStyle::State_Enabled;
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &boxOption, painter, opt.widget);

        bool hasUpdate = false;
        if (const QVariant varPkgs = index.data(Qt::UserRole + 4); varPkgs.canConvert<QList<PkgInfo> >()) {
            const auto pkgList = varPkgs.value<QList<PkgInfo> >();
            if (!pkgList.isEmpty()) {
                bool attachTesting = false;
                for (const QWidget *w = opt.widget; w; w = w->parentWidget()) {
                    const QVariant val = w->property("attachTesting");
                    if (val.isValid()) {
                        attachTesting = val.toBool();
                        break;
                    }
                }

                bool rowIsTestingOnly = true;
                for (const auto &p: pkgList) {
                    if (!p.repoName.contains(SLACK_TESTING)) {
                        rowIsTestingOnly = false;
                        break;
                    }
                }

                auto getRepoPriority = [attachTesting](const QString &repo, bool testingOnly) -> int {
                    if ((attachTesting || testingOnly) && repo.contains(SLACK_TESTING))
                        return 3;
                    if (repo.contains(SLACK_PATCHES))
                        return 2;
                    if (repo.compare(SLACK_OFICIAL) == 0)
                        return 1;
                    return 0;
                };

                auto isVersionNewer = [](const QString &v1, const QString &v2) -> bool {
                    const QVersionNumber ver1 = QVersionNumber::fromString(v1);
                    const QVersionNumber ver2 = QVersionNumber::fromString(v2);
                    if (!ver1.isNull() && !ver2.isNull())
                        return QVersionNumber::compare(ver1, ver2) > 0;
                    return v1 > v2;
                };

                const PkgInfo *installedCandidate = nullptr;
                for (const auto &p: pkgList) {
                    if (p.isInstalled) {
                        installedCandidate = &p;
                        break;
                    }
                }

                const PkgInfo *bestAvailable = nullptr;
                int maxPriority = -1;

                for (const auto &p: pkgList) {
                    if (p.isInstalled)
                        continue;

                    if (p.repoName.contains(SLACK_TESTING) && !attachTesting && !rowIsTestingOnly)
                        continue;

                    const int priority = getRepoPriority(p.repoName, rowIsTestingOnly);
                    if (priority > maxPriority) {
                        maxPriority = priority;
                        bestAvailable = &p;
                    } else if (priority == maxPriority && bestAvailable) {
                        // Se estiverem na mesma prioridade, escolhe a versão mais nova
                        if (isVersionNewer(p.version, bestAvailable->version)) {
                            bestAvailable = &p;
                        }
                    }
                }

                if (installedCandidate && bestAvailable) {
                    const int installedPriority = getRepoPriority(installedCandidate->repoName, rowIsTestingOnly);

                    if (maxPriority > installedPriority)
                        hasUpdate = true;

                    else if (maxPriority == installedPriority) {
                        if (bestAvailable->version != installedCandidate->version &&
                            isVersionNewer(bestAvailable->version, installedCandidate->version)) {
                            hasUpdate = true;
                        }
                    }
                }
            }
        }

        int textLeftOffset = 0;

        if (hasUpdate) {
            QIcon updateIcon(DefaultPath().defaultPath("icons/available.svg"));

            const QRect iconRect(opt.rect.x() + margin, opt.rect.y() + (opt.rect.height() - iconSize) / 2,
                                 iconSize, iconSize);

            updateIcon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::On);

            textLeftOffset = margin + iconSize + (margin / 2);
        }

        QRect textRect = opt.rect.adjusted(textLeftOffset, 0, 0, 0);
        textRect.setRight(buttonRect.left() - margin);

        const QPalette::ColorRole textRole =
                opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text;

        style->drawItemText(painter, textRect, Qt::AlignVCenter | Qt::AlignLeft, opt.palette,
                            opt.state.testFlag(QStyle::State_Enabled), opt.text, textRole);

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
                const int buttonSize = qMax(height, option.rect.height() - margin * 2);

                const QRect buttonRect(
                    option.rect.x() + option.rect.width() - margin - buttonSize,
                    option.rect.y() + margin,
                    buttonSize, buttonSize
                );

                if (buttonRect.contains(mouseEvent->pos())) {
                    auto *pw = const_cast<QWidget *>(option.widget);

                    if (auto *view = qobject_cast<QAbstractItemView *>(const_cast<QWidget *>(option.widget)))
                        view->setCurrentIndex(index);

                    const QString currentVersion = index.data(Qt::DisplayRole).toString();

                    QMenu menu(pw);
                    for (int i = 0; i < versions.size(); ++i) {
                        QAction *action = menu.addAction(versions.at(i));
                        action->setData(i);

                        if (versions.at(i) == currentVersion) {
                            action->setCheckable(true);
                            action->setChecked(true);
                        }
                    }

                    menu.adjustSize();

                    QPoint globalPos = QCursor::pos();

                    if (pw) {
                        const QPoint buttonBottomRight(buttonRect.x() + buttonRect.width(),
                                                       buttonRect.y() + buttonRect.height());

                        globalPos = pw->mapToGlobal(buttonBottomRight);
                        globalPos.setX(globalPos.x() - menu.sizeHint().width() - buttonRect.width());
                        globalPos.setY(globalPos.y() + 3);
                    }

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
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
