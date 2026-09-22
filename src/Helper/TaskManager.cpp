/**
 * @file TaskManager.cpp
 * @brief Handles run command through a terminal and provides bidirectional communication between the helper and GUI.
 */

#include <QSocketNotifier>

#include <TaskManager.hpp>
#include <SlackwareDefines.hpp>

#include <cerrno>
#include <pty.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

/**
 * @brief Constructs a task manager responsible for running commands inside a PTY.
 * @param s Client communication socket.
 * @param parent Parent object.
 */
TaskManager::TaskManager(QLocalSocket *s, QObject *parent) : QObject(parent) {
    socket = s;
}

/**
 * @brief Starts a command attached to a pseudo-terminal.
 * @param cmd Executable name.
 * @param args Command arguments.
 */
void TaskManager::startCommand(const QString &cmd, const QStringList &args) {
    if (childPid > 0)
        return;

    childPid = forkpty(&masterFd, nullptr, nullptr, nullptr);

    if (childPid < 0) {
        socket->write(ERROR + QByteArray("Failed to create PTY") + SEP);
        childPid = -1;
        return;
    }

    if (childPid == 0) {
        termios t{};

        if (tcgetattr(STDIN_FILENO, &t) == 0) {
            t.c_lflag &= ~ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &t);
        }

        QVector<QByteArray> cArgs;
        cArgs.push_back(cmd.toUtf8());

        for (const auto &arg: args)
            cArgs.push_back(arg.toUtf8());

        QVector<char *> argv;
        argv.reserve(cArgs.size() + 1);

        for (auto &arg: cArgs)
            argv.push_back(arg.data());

        argv.push_back(nullptr);
        execvp(argv[0], argv.data());

        _exit(EXIT_FAILURE);
    }

    if (notifier) {
        notifier->deleteLater();
        notifier = nullptr;
    }

    notifier = new QSocketNotifier(masterFd, QSocketNotifier::Read, this);

    connect(notifier, &QSocketNotifier::activated, this, [this] {
        char buffer[4096];
        const int bytes = static_cast<int>(read(masterFd, buffer, sizeof(buffer)));

        if (!cancelled && bytes > 0)
            socket->write(QByteArray(TASK, TASK_SIZE) + QByteArray(buffer, bytes) + SEP);

        if (bytes == 0 || (bytes < 0 && errno == EIO)) {
            int status = 0;
            waitpid(childPid, &status, 0);
            cancelled = false;

            if (notifier) {
                notifier->setEnabled(false);
                notifier->deleteLater();
                notifier = nullptr;
            }

            close(masterFd);

            masterFd = -1;
            childPid = -1;
            emit commandFinished();
            return;
        }

        int status = 0;

        if (waitpid(childPid, &status, WNOHANG) == childPid) {
            cancelled = false;

            if (notifier) {
                notifier->setEnabled(false);
                notifier->deleteLater();
                notifier = nullptr;
            }

            close(masterFd);

            masterFd = -1;
            childPid = -1;
            emit commandFinished();
        }
    });
}

/**
 * @brief Sends user input to the running process.
 * @param input Input data.
 */
void TaskManager::sendInput(const QByteArray &input) const {
    if (masterFd < 0)
        return;

    QByteArray data = input;

    if (!data.endsWith('\n'))
        data.append('\n');

    write(masterFd, data.constData(), data.size());
}

/**
 * @brief Sends an interrupt signal to the running process.
 */
void TaskManager::cancelCommand() {
    if (childPid > 0) {
        cancelled = true;
        kill(childPid, SIGINT);
    }
}
