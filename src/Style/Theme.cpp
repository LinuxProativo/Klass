/**
 * @file Theme.cpp
 * @brief Implementation of the Theme class to manage application styling.
 */

#include <QColor>
#include <QPalette>
#include <QStyleFactory>

#include <Fonts.hpp>
#include <Theme.hpp>

/**
 * @brief Applies the custom theme settings to the application.
 */
void Theme::applyTheme() {
    if (QStyle *baseEngine = QStyleFactory::create("Fusion"))
        QApplication::setStyle(baseEngine);

    QPalette palette = QApplication::palette();
    QColor originalActiveColor = palette.color(QPalette::Active, QPalette::Highlight);
    QColor originalInactiveColor = palette.color(QPalette::Inactive, QPalette::Highlight);

    originalActiveColor.setAlpha(50);
    originalInactiveColor.setAlpha(50);

    palette.setColor(QPalette::Active, QPalette::Highlight, originalActiveColor);
    palette.setColor(QPalette::Inactive, QPalette::Highlight, originalInactiveColor);

    QApplication::setPalette(palette);
    QApplication::setStyle(new NoFocusStyle);
    QApplication::setFont(Fonts::getSystemFont());
}
