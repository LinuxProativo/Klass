/**
 * @file StatusIcons.cpp
 * @brief Implementation of the upfront icon initialization for branchless retrieval.
 */

#include <QPalette>
#include <Icon.hpp>
#include <SlackwareDefines.hpp>
#include <StatusIcons.hpp>

bool StatusIcons::initialized = false;
std::array<QIcon, static_cast<size_t>(PkgStatus::Count)> StatusIcons::icons{};

/**
 * @brief Pre-allocates and generates all icons. Must be called once at application startup.
 */
void StatusIcons::init() {
    if (initialized) return;

    Icon::IconManager iconFactory;
    iconFactory.setSize(24);

    const QIcon availableIcon = iconFactory.getIcon(Icon::Id::Unselect, QPalette().color(QPalette::Dark));

    icons[static_cast<size_t>(PkgStatus::Reset)] = availableIcon;
    icons[static_cast<size_t>(PkgStatus::Available)] = availableIcon;
    icons[static_cast<size_t>(PkgStatus::Installed)] = iconFactory.getIcon(Icon::Id::Installed, COLOR_BLUE);
    icons[static_cast<size_t>(PkgStatus::Excluded)] = iconFactory.getIcon(Icon::Id::MarkBlock, COLOR_YELLOW);
    icons[static_cast<size_t>(PkgStatus::PendingInstall)] = iconFactory.getIcon(Icon::Id::MarkInstall, COLOR_GREEN);
    icons[static_cast<size_t>(PkgStatus::PendingUpdate)] = iconFactory.getIcon(Icon::Id::MarkUpdate, COLOR_PURPLE);
    icons[static_cast<size_t>(PkgStatus::PendingReinstall)] = iconFactory.getIcon(Icon::Id::MarkReinstall, COLOR_BLUE);
    icons[static_cast<size_t>(PkgStatus::PendingRemove)] = iconFactory.getIcon(Icon::Id::MarkRemove, COLOR_RED);
    initialized = true;
}

/**
 * @brief Extremely fast, branchless inline getter for pre-generated icons.
 * @param status The target package status enum value.
 * @return Reference to the associated QIcon instance.
 */
const QIcon & StatusIcons::statusIcon(const PkgStatus status) {
    Q_ASSERT_X(static_cast<size_t>(status) < static_cast<size_t>(PkgStatus::Count),
               "StatusIcons::statusIcon", "Status enum out of bounds");

    return icons[static_cast<size_t>(status)];
}
