/**
 * @file TaskManager.hpp
 * @brief header for PTY session system.
 */

#ifndef TASKMANAGER_HPP
#define TASKMANAGER_HPP

#include <QLocalSocket>
#include <QObject>
#include <QSocketNotifier>

/**
 * @class TaskManager
 * @brief Manages execution of system commands inside a PTY session.
 */
class TaskManager : public QObject {
    Q_OBJECT

public:
    explicit TaskManager(QLocalSocket *s, QObject *parent = nullptr);

    void startCommand(const QString &cmd, const QStringList &args);

    void sendInput(const QByteArray &input) const;

    void cancelCommand();

    signals:
        void commandFinished();

private:
    QLocalSocket *socket{};
    QSocketNotifier *notifier{};

    bool cancelled{false};
    int masterFd{-1};
    pid_t childPid{-1};
};

#endif
