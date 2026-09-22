/**
 * @file CenterDelegate.cpp
 * @brief Implementation of the CenterDelegate class for custom cell alignment and glyph styling.
 */

#include <QApplication>
#include <QPainter>

#include <CenterDelegate.hpp>
#include <RepositoryTabUtils.hpp>
#include <SlackwareDefines.hpp>

/**
 * @brief Constructs a CenterDelegate instance.
 * @param parent The parent object, or nullptr if none.
 */
CenterDelegate::CenterDelegate(QObject *parent): QStyledItemDelegate(parent) {}

/**
 * @brief Renders the item using a customized layout option, font size, and color palette based on text content.
 * @param painter The painter object used to draw the item.
 * @param option The style options for the item being rendered.
 * @param index The model index identifying the item in the table.
 */
void CenterDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const auto icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (icon.isNull()) {
        QStyledItemDelegate::paint(painter, opt, index);
        return;
    }

    const QSize size = icon.actualSize(opt.rect.size());

    const QRect iconRect(
        opt.rect.left() + (opt.rect.width() - size.width()) / 2,
        opt.rect.top() + (opt.rect.height() - size.height()) / 2,
        size.width(),
        size.height()
    );

    const bool isPrioritized = index.data(Qt::UserRole + 3).toBool();
    const int packageStatusVal = index.model()->data(index, RepositoryTabUtils::PackageStatusRole).toInt();
    const bool isExcluded = (packageStatusVal == static_cast<int>(PkgStatus::Excluded));
    const bool isInstalled = index.siblingAtColumn(1).data(Qt::UserRole + 1).toBool();

    QColor targetColor{};
    bool applyColor = false;

    if (isPrioritized) {
        targetColor = QColor(COLOR_PURPLE);
        applyColor = true;
    } else if (isExcluded && !isInstalled) {
        targetColor = QPalette().color(QPalette::Dark);
        applyColor = true;
    }

    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    if (applyColor) {
        QPixmap pixmap = icon.pixmap(size, QIcon::Normal, QIcon::On);
        QPainter pixmapPainter(&pixmap);
        pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        pixmapPainter.fillRect(pixmap.rect(), targetColor);
        pixmapPainter.end();
        painter->drawPixmap(iconRect, pixmap);
    } else {
        icon.paint(painter, iconRect, Qt::AlignCenter);
    }
}
