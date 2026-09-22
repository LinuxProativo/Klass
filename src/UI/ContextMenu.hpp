/**
 * @file ContextMenu.hpp
 * @brief Custom QMenu implementation with a proxy style for enlarged 24px icons.
 */

#ifndef CONTEXTMENU_HPP
#define CONTEXTMENU_HPP

#include <QMenu>
#include <QProxyStyle>

/**
 * @class IconContextMenu
 * @brief Proxy style that forces small icon metrics to 24px for associated widgets.
 */
class IconContextMenu : public QProxyStyle {
    Q_OBJECT

public:
    /**
     * @brief Overrides pixel metrics to force PM_SmallIconSize to 24px.
     * @param metric Style metric to query.
     * @param option Style option parameters.
     * @param widget Widget associated with the style request.
     * @return Metric size in pixels.
     */
    int pixelMetric(const PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override {
        if (metric == PM_SmallIconSize)
            return 24;

        return QProxyStyle::pixelMetric(metric, option, widget);
    }
};

/**
 * @class ContextMenu
 * @brief Custom QMenu that automatically applies IconContextMenuStyle upon creation.
 */
class ContextMenu : public QMenu {
    Q_OBJECT

public:
    explicit ContextMenu(QWidget *parent = nullptr);

    explicit ContextMenu(const QString &title, QWidget *parent = nullptr);

    ContextMenu *addContextMenu(const QString &title);
};

#endif
