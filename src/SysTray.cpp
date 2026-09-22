/**
 * @file SysTray.cpp
 * @brief Manages the system tray icon and background application behavior.
 */

#include <QTimer>

#include <DefaultPath.hpp>
#include <SysTray.hpp>

/**
 * @brief Constructor for the tray manager.
 * @param parent - The parent widget, used to control the visibility and geometry of the main window.
 * @param helperSocket - Local socket used to send application-wide exit/quit commands.
 * @param settingsManager Manager responsible for saving and loading application configurations.
 */
SysTray::SysTray(QWidget *parent, QLocalSocket *helperSocket, SettingsManager *settingsManager) : QObject(parent) {
    trayMenu = new QMenu();
    const auto *quitAction = trayMenu->addAction(tr("Quit"));

    connect(quitAction, &QAction::triggered, [helperSocket, parent, settingsManager]() {
        if (helperSocket && helperSocket->state() == QLocalSocket::ConnectedState) {
            helperSocket->write("QUIT");
        }

        if (!parent->isMaximized()) // Se tiver maximizado dá zica
            settingsManager->windowGeometry(parent->geometry());

        QTimer::singleShot(100, qApp, &QCoreApplication::quit);
    });

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(DefaultPath().defaultPath("klass.png")));
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip(tr("Klass Package Manager"));

    connect(trayIcon, &QSystemTrayIcon::activated, [parent](const QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger && parent) {
            if (parent->isVisible() && parent->isActiveWindow()) {
                parent->hide();
            } else {
                parent->show();
                parent->activateWindow();
            }
        }
    });

    trayIcon->show();
}
