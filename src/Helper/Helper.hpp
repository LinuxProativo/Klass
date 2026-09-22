/**
 * @file Helper.hpp
 * @brief Header for Helper Daemon.
 */

#ifndef HELPER_HPP
#define HELPER_HPP

#include <QLocalServer>
#include <QObject>
#include <QStandardPaths>
#include <QTimer>

#include <HelperUtils.hpp>

/**
 * @class Helper
 * @brief Root-level daemon responsible for managing privileged operations.
 */
class Helper final : public QObject {
    Q_OBJECT

public:
    explicit Helper(QObject *parent = nullptr);

private:
    static bool validateClient(const QLocalSocket *socket);

    QLocalServer *server{};
    QLocalSocket *clientSocket{};
    QTimer *idleTimer{};

    HelperUtils *helperUtils{};
};

#endif
