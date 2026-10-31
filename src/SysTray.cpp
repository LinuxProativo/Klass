/**
 * @file SysTray.cpp
 * @brief Implementation of standalone SysTray decoupled from the main window.
 */

#include <QApplication>
#include <SysTray.hpp>

/**
 * @brief Constructor for the standalone tray manager.
 * @param parent Optional parent QObject.
 */
SysTray::SysTray(QObject *parent) : QObject(parent) {
#undef signals
    notify_init("Klass");
#define signals public

    trayMenu = new QMenu();
    const auto *openAction = trayMenu->addAction(tr("Open/Close Klass"));
    trayMenu->addSeparator();
    const auto *quitAction = trayMenu->addAction(tr("Quit"));

    connect(openAction, &QAction::triggered, this, &SysTray::openRequested);
    connect(quitAction, &QAction::triggered, this, &SysTray::quitRequested);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(defaultIcon);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip(tr("Klass Package Manager"));

    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](const QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            emit openRequested();
    });

    trayIcon->show();
}

/**
 * @brief Restores the default system tray icon and tooltip.
 */
void SysTray::resetIcon() const {
    trayIcon->setIcon(defaultIcon);
    trayIcon->setToolTip(tr("Klass Package Manager"));
}

/**
 * @brief Updates tray icon state and dispatches desktop notification when updates are found.
 * @param available True if newer database/packages are detected.
 */
void SysTray::setUpdateAvailable(const bool available) const {
    if (available) {
        const QString iconPath = DefaultPath::defaultPath("icons/available.svg");
        trayIcon->setIcon(QIcon(iconPath));
        trayIcon->setToolTip(tr("Klass Package Manager - Database Updates Available!"));

        notify_send(tr("Database Update Available").toUtf8().constData(),
                    tr("A newer Slackware package database is available.").toUtf8().constData(),
                    iconPath.toUtf8().constData());
    } else {
        resetIcon();
    }
}
