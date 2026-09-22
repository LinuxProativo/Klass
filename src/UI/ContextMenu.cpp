/**
 * @file ContextMenu.cpp
 * @brief Implementation of ContextMenu and IconContextMenuStyle.
 */

#include <ContextMenu.hpp>

/**
 * @brief Constructs a ContextMenu instance.
 * @param parent Parent widget.
 */
ContextMenu::ContextMenu(QWidget *parent) : QMenu(parent) {
    this->setStyle(new IconContextMenu());
}

/**
 * @brief Constructs a ContextMenu instance with a title.
 * @param title Title of the menu.
 * @param parent Parent widget.
 */
ContextMenu::ContextMenu(const QString &title, QWidget *parent) : QMenu(title, parent) {
    this->setStyle(new IconContextMenu());
}

/**
 * @brief Creates and adds a new ContextMenu sub-menu.
 * @param title Title of the sub-menu.
 * @return Pointer to the created ContextMenu.
 */
ContextMenu *ContextMenu::addContextMenu(const QString &title) {
    auto *subMenu = new ContextMenu(title, this);
    addMenu(subMenu);
    return subMenu;
}
