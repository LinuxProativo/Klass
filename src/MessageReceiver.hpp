/**
 * @file MessageReceiver.hpp
 * @brief Header for communication using MessageReceiver.
 */

#ifndef MESSAGERECEIVER_HPP
#define MESSAGERECEIVER_HPP

#include <QObject>

/**
 * @class MessageReceiver
 * @brief Manages the reception of messages from secondary instances to maintain single-instance integrity.
 */
class MessageReceiver : public QObject {
    Q_OBJECT

public slots:
    void receivedMessage(int instanceId, const QByteArray &message);

signals:
    void arg(const QString &lst);
};

#endif
