/**
 * @file Buttons.cpp
 * @brief This file implements a custom QPushButton subclass that provides size-scaling hover effects.
 */

#include <Buttons.hpp>

/**
 * @brief Constructs a custom button, configuring transparency, tooltips, and pre-rendering state icons.
 * @param icoName The identifier for the icon to be displayed.
 * @param tooltip Optional text to be displayed on hover. Defaults to an empty string.
 * @param size The base size of the button and icon.
 * @param max The additional size applied during hover effects.
 */
Buttons::Buttons(const Icon::Id icoName, const QString &tooltip, const int size,
                 const int max) : maxsize(max), num(size) {
    Icon::IconManager iconManager(num, maxsize);

    iconManager.setSize(num + maxsize);
    upperIcon = iconManager.getIcon(icoName);

    iconManager.setSize(num);
    normalIcon = iconManager.getIcon(icoName);

    if (!tooltip.isEmpty())
        this->setToolTip(tooltip);

    this->setFocusPolicy(Qt::NoFocus);
    this->setFixedSize(num + maxsize, num + maxsize);
    this->setStyleSheet(QStringLiteral("QPushButton { border: none; background-color: transparent; }"));
    updateIcon(false);
}

/**
 * @brief Refreshes the icon rendering and dynamically resizes the button layout payload.
 * @param hovered True scales up the button and assigns the larger icon; false shrinks it.
 */
void Buttons::updateIcon(const bool hovered) {
    const int currentSize = hovered ? (num + maxsize) : num;
    this->setIconSize(QSize(currentSize, currentSize));
    this->setIcon(hovered ? upperIcon : normalIcon);
}

/**
 * @brief Handles hover entry to apply icon color and size expansion.
 * @param event The enter event instance.
 */
void Buttons::enterEvent(QEnterEvent *event) {
    updateIcon(true);
    QPushButton::enterEvent(event);
}

/**
 * @brief Handles hover exit to restore icon color and base size.
 * @param event The leave event instance.
 */
void Buttons::leaveEvent(QEvent *event) {
    updateIcon(false);
    QPushButton::leaveEvent(event);
}

/**
 * @brief Handles mouse press to provide visual feedback by shrinking the icon.
 * @param event The mouse event instance.
 */
void Buttons::mousePressEvent(QMouseEvent *event) {
    updateIcon(false);
    QPushButton::mousePressEvent(event);
}

/**
 * @brief Handles mouse release to restore the button to its larger hover size.
 * @param event The mouse event instance.
 */
void Buttons::mouseReleaseEvent(QMouseEvent *event) {
    updateIcon(true);
    QPushButton::mouseReleaseEvent(event);
}
