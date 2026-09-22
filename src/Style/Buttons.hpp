/**
 * @file Buttons.hpp
 * @brief Header for custom interactive buttons with hover animations.
 */

#ifndef BUTTONS_HPP
#define BUTTONS_HPP

#include <QEnterEvent>
#include <QIcon>
#include <QPushButton>

#include <Icon.hpp>

/**
 * @class Buttons
 * @brief Custom QPushButton implementation with support for dynamic icon-based styling and hover scaling.
 */
class Buttons : public QPushButton {
    Q_OBJECT

public:
    explicit Buttons(Icon::Id icoName, const QString &tooltip = {}, int size = defIconSize, int max = defMax);

    void updateIcon(bool hovered);

protected:
    void enterEvent(QEnterEvent *event) override;

    void leaveEvent(QEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QIcon normalIcon{}, upperIcon{};
    int maxsize, num;
};

#endif
