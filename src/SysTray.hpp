/**
 * @file SysTray.hpp
 * @brief Header for display SysTray.
 */

#ifndef SYSTRAY_HPP
#define SYSTRAY_HPP

#include <QLocalSocket>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

#include <SettingsManager.hpp>

#undef signals
#include <libnotify/notify.h>

/**
 * @brief Displays a system notification popup.
 * @param title The main headline or title of the notification.
 * @param msg The body text containing the detailed message content.
 * @param ico The file path or system name of the icon to display.
 */
inline void notify_send(const char *title, const char *msg, const char *ico) {
    NotifyNotification *n = notify_notification_new(title, msg, ico);
    notify_notification_set_timeout(n, 5000);

    if (!notify_notification_show(n, nullptr)) return;
    g_object_unref(G_OBJECT(n));
}

#define signals public

/**
 * @class SysTray
 * @brief Handles the application's presence in the system tray.
 */
class SysTray : public QObject {
    Q_OBJECT

public:
    explicit SysTray(QWidget *parent = {}, QLocalSocket *helperSocket = {}, SettingsManager *settingsManager = {});

    [[nodiscard]] QSystemTrayIcon *getTray() const { return trayIcon; }

    void resetIcon() const { trayIcon->setIcon(trayicon); }
private:
    QIcon trayicon = QIcon(DefaultPath().defaultPath("icons/tray.svg"));

    QSystemTrayIcon *trayIcon{};
    QMenu *trayMenu{};
};

#endif
