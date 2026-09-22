/**
 * @file Theme.hpp
 * @brief Header for custom theme.
 */

#ifndef THEME_HPP
#define THEME_HPP

#include <QObject>
#include <QProxyStyle>
#include <QApplication>

/**
 * @class NoFocusStyle
 * @brief A custom proxy style that disables the default focus rectangle drawing.
 */
class NoFocusStyle : public QProxyStyle {
public:
    void drawPrimitive(const PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget) const override {
        if (element == PE_FrameFocusRect) return;
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

/**
 * @class Theme
 * @brief Class responsible for managing and applying application-wide styles.
 */
class Theme : public QObject {
    Q_OBJECT

public:
    static void applyTheme();
};

#endif
