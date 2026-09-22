/**
 * @file Icon.hpp
 * @brief Header for custom application icons.
 */

#ifndef ICON_HPP
#define ICON_HPP

#include <QIcon>
#include <QStyle>
#include <QStringView>

inline constexpr int defIconSize = 31;
inline constexpr int defMax = 3;

namespace Icon {
    /**
     * @enum Id
     * @brief Custom identifiers for context menu and button icons.
     */
    enum class Id {
        Black = 0,
        About,
        Update,
        Add,
        Reset,
        Up,
        Exception,
        Apply,
        Changelog,
        Settings,
        Installed,
        Unselect,
        MarkInstall,
        MarkRemove,
        MarkUpdate,
        MarkReinstall,
        MarkRollback,
        MarkPriority,
        MarkBlock,
        Count
    };

    /**
     * @class IconManager
     * @brief Manages application icon retrieval, theme-based coloring, and dynamic scaling.
     */
    class IconManager {
    public:
        explicit IconManager(int size = defIconSize, int max = defMax);

        void setSize(const int size) { num = size; }

        [[nodiscard]] QIcon getIcon(Id iconId, const QColor &color = QPalette().color(QPalette::WindowText)) const;

    private:
        [[nodiscard]] QString selectIconPath(Id iconId) const;

        [[nodiscard]] QIcon applyContrast(const QIcon &icon, const QColor &color) const;

        int num, maxsize;
        QString baseIconPath{};
    };
}

#endif
