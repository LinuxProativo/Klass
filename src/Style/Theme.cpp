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
    QStyle *pureFusion = QStyleFactory::create("Fusion");
    const QPalette rawPalette = pureFusion->standardPalette();

    QColor originalActiveColor = rawPalette.color(QPalette::Active, QPalette::Highlight);
    QColor originalInactiveColor = rawPalette.color(QPalette::Inactive, QPalette::Highlight);

    originalActiveColor.setAlpha(50);
    originalInactiveColor.setAlpha(50);

    QApplication::setPalette(rawPalette);
    QApplication::setStyle(new NoFocusStyle(pureFusion));
    QApplication::setFont(Fonts::getSystemFont());
}
