/**
 * @file Icon.cpp
 * @brief This file implements the application's icon management system.
 */

#include <QApplication>
#include <QPainter>
#include <QFileInfo>

#include <array>

#include <Debug.hpp>
#include <DefaultPath.hpp>
#include <Icon.hpp>

namespace Icon {
    /**
     * @struct IconMapping
     * @brief Maps an internal enum identifier to its filesystem name and Qt fallback style.
     */
    struct IconMapping {
        QStringView fileName;
        QStyle::StandardPixmap fallback;
    };

    /**
     * @brief O(1) Lookup table replacing the slow and type-unsafe QVariantList.
     * Computed at compile-time for absolute maximum performance.
     */
    static constexpr std::array<IconMapping, static_cast<size_t>(Id::Count)> iconTable = {
        {
            {u"", QStyle::SP_CustomBase},
            {u"about", QStyle::SP_MessageBoxQuestion},
            {u"update", QStyle::SP_BrowserReload},
            {u"add", QStyle::SP_FileDialogNewFolder},
            {u"reset", QStyle::SP_DialogResetButton},
            {u"up", QStyle::SP_ArrowUp},
            {u"rules", QStyle::SP_BrowserStop},
            {u"apply", QStyle::SP_DialogApplyButton},
            {u"changelog", QStyle::SP_FileIcon},
            {u"settings", QStyle::SP_ComputerIcon},
            {u"check_installed", QStyle::SP_CustomBase},
            {u"check_unselect", QStyle::SP_CustomBase},
            {u"check_install", QStyle::SP_CustomBase},
            {u"check_remove", QStyle::SP_CustomBase},
            {u"check_update", QStyle::SP_CustomBase},
            {u"check_reinstall", QStyle::SP_CustomBase},
            {u"check_rollback", QStyle::SP_CustomBase},
            {u"check_priority", QStyle::SP_CustomBase},
            {u"check_block", QStyle::SP_CustomBase}
        }
    };

    /**
     * @brief Constructs the IconManager, caching the base icon directory.
     * @param size The base size for the icons.
     * @param max The expansion size used for hover effects.
     */
    IconManager::IconManager(const int size, const int max) : num(size), maxsize(max) {
        baseIconPath = DefaultPath().defaultPath("icons");
    }

    /**
     * @brief Resolves the absolute path for a requested SVG icon from the filesystem.
     * @param iconId The enum identifier of the icon.
     * @return The absolute file path, or an empty string if the file doesn't exist.
     */
    QString IconManager::selectIconPath(const Id iconId) const {
        if (baseIconPath.isEmpty()) return {};
        const auto idx = static_cast<size_t>(iconId);

        if (const QString ico = QStringLiteral("%1/%2.svg").arg(baseIconPath, iconTable[idx].fileName);
            QFileInfo::exists(ico)) {
            Debug::Debug().msg("Select Icon", "Icon", {ico, Debug::Cyan});
            return ico;
        }

        return {};
    }

    /**
     * @brief Colorizes an icon using standard QPainter composition modes.
     * @param icon The source icon to be color-masked.
     * @param color The requested color to fill the icon boundaries.
     * @return A newly painted QIcon containing the colorized pixmap.
     */
    QIcon IconManager::applyContrast(const QIcon &icon, const QColor &color) const {
        QPixmap themed_pixmap(num, num);
        themed_pixmap.fill(Qt::transparent);

        QPainter painter(&themed_pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::LosslessImageRendering, true);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.drawPixmap(0, 0, icon.pixmap(QSize(num, num)));
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(themed_pixmap.rect(), color);
        painter.end();

        return QIcon(themed_pixmap);
    }

    /**
     * @brief Generates the final icon, prioritizing local SVG files or falling back to system icons.
     * @param iconId The enum identifier of the icon.
     * @param color The color to apply to the icon.
     * @return The final QIcon ready to be displayed in the UI.
     */
    QIcon IconManager::getIcon(const Id iconId, const QColor &color) const {
        const QString ico = selectIconPath(iconId);

        if (ico.isEmpty()) {
            const auto idx = static_cast<size_t>(iconId);
            return QApplication::style()->standardIcon(iconTable[idx].fallback);
        }

        return applyContrast(QIcon(ico), color);
    }
}
