/**
 * @file HelperController.cpp
 * @brief Controller for managing the helper process and communication between the GUI and privileged operations.
 */

#include <QCoreApplication>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>

#include <Helper.hpp>
#include <HelperController.hpp>
#include <SlackwareDefines.hpp>

/**
 * @brief Constructs the helper controller.
 * @param dialog Terminal dialog.
 * @param parent Parent object.
 */
HelperController::HelperController(Terminal *dialog, QObject *parent) : QObject(parent), terminalDialog(dialog) {
    debug = new Debug::Debug();

    proc = new Process(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &HelperController::onFinished);

    helperSocket = new QLocalSocket(this);
    helperSocket->setSocketOptions(QLocalSocket::AbstractNamespaceOption);
    connect(helperSocket, &QLocalSocket::errorOccurred, this, &HelperController::onHelperSocketError);
    connect(helperSocket, &QLocalSocket::connected, this, &HelperController::onHelperConnected);
    connect(helperSocket, &QLocalSocket::readyRead, this, &HelperController::onHelperReadyRead);

    connect(dialog, &Terminal::inputEntered, this, [this](const QByteArray &text) {
        helperSocket->write(QByteArray(INPUT_CHAR) + text + SEP);
    });
    connect(dialog, &Terminal::cancelOperationRequest, this, [this] {
        helperSocket->write(QByteArray("CANCEL_OPERATION") + SEP);
    });
    connect(dialog, &Terminal::cancelRequested, this, [this] {
        helperSocket->write(QByteArray("CANCEL") + SEP);
    });
}

/**
 * @brief Starts or connects to the helper process.
 * @param pending Pending helper action.
 */
void HelperController::setupHelper(const PendingAction pending) {
    pendingAction = pending;
    helperShutdownRequested = true;
    if (helperSocket->state() == QLocalSocket::ConnectedState) {
        debug->msg("Helper already connected", "HelperController", {Debug::Orange});
        emit helperReady();
        return;
    }

    QTimer::singleShot(350, this, [this] {
        helperShutdownRequested = false;
        countConnect = 0;
        helperSocket->connectToServer(SOCKET_PATH, QIODevice::ReadWrite);
    });

    if (helperSocket->waitForConnected(300)) {
        emit helperReady();
        debug->msg("Connected to existing helper", "HelperController");
        return;
    }
    const QString appPath = QCoreApplication::applicationFilePath();
    QStringList envPrefix;

    const QString display = qEnvironmentVariable("DISPLAY");
    const QString xauth = qEnvironmentVariable("XAUTHORITY");
    const QString waylandDisplay = qEnvironmentVariable("WAYLAND_DISPLAY");
    const QString xdgRuntime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    const QString xdgSession = qEnvironmentVariable("XDG_SESSION_TYPE");

    if (!display.isEmpty()) envPrefix << "DISPLAY=" + display;
    if (!xauth.isEmpty()) envPrefix << "XAUTHORITY=" + xauth;
    if (!waylandDisplay.isEmpty()) envPrefix << "WAYLAND_DISPLAY=" + waylandDisplay;
    if (!xdgRuntime.isEmpty()) envPrefix << "XDG_RUNTIME_DIR=" + xdgRuntime;
    if (!xdgSession.isEmpty()) envPrefix << "XDG_SESSION_TYPE=" + xdgSession;

    QStringList args;
    args << "env" << envPrefix << appPath << "--helper";

    proc->start("pkexec", args);
    debug->msg("pkexec started", "HelperController");
}

/**
 * @brief Handles helper socket connection errors.
 * @param error Socket error.
 */
void HelperController::onHelperSocketError(const QLocalSocket::LocalSocketError error) {
    Q_UNUSED(error)

    if (helperShutdownRequested)
        return;

    if (countConnect >= 150) {
        debug->msg("Helper connection timeout", "HelperController", {Debug::LightRed});

        QMessageBox::information(
            nullptr,
            tr("Authentication Timeout"),
            tr("Authentication timed out after 30 seconds."));

        if (proc && proc->state() == QProcess::Running)
            proc->kill();

        countConnect = 0;
        return;
    }

    ++countConnect;
    QTextStream(stdout) << "\rRetrying " << countConnect;

    QTimer::singleShot(200, this, [this] {
        helperSocket->abort();
        helperSocket->connectToServer(SOCKET_PATH, QIODevice::ReadWrite);
    });
}

/**
 * @brief Called when helper socket connects successfully.
 */
void HelperController::onHelperConnected() {
    if (helperSocket->state() == QLocalSocket::ConnectedState) {
        debug->msg("Connected to helper", "HelperController", {Debug::LightGreen});
        emit helperReady();
    }

    helperShutdownRequested = false;
    countConnect = 0;
}

/**
 * @brief Handles pkexec process completion.
 * @param exitCode Process exit code.
 * @param status Process exit status.
 */
void HelperController::onFinished(const int exitCode, const QProcess::ExitStatus status) {
    Q_UNUSED(status)
    debug->msg("pkexec exited: " + QString::number(exitCode), "HelperController");
    helperShutdownRequested = true;

    if (const QString err = proc->readAllStandardError(); !err.isEmpty())
        debug->msg("pkexec stderr: " + err, "HelperController");
}

/**
 * @brief Processes messages received from helper.
 */
void HelperController::onHelperReadyRead() {
    for (const QByteArray data = helperSocket->readAll(); const QByteArray &line : data.split(SEP)) {
        if (line.isEmpty()) continue;

        if (line.startsWith(TASK)) {
            terminalDialog->writeOutput(line.mid(TASK_SIZE));
        } else if (line.startsWith(OUTPUT)) {
            terminalDialog->writeOutput(line.mid(OUTPUT_SIZE) + "\n");
        } else if (line.startsWith(LOG)) {
            debug->msg(QString::fromUtf8(line.mid(LOG_SIZE)), "Helper");
        } else if (line == "DONE") {
            terminalDialog->writeOutput("\n--- DONE ---\n");
            terminalDialog->setFinished(true);
        } else if (line == "UPDATE_DONE") {
            emit updateFinished();
        } else if (line == "UPDATE_RULES") {
            emit updateRules();
        } else if (line == "TRANSACTION_DONE") {
            emit transactionFinished();
        } else if (line.startsWith(ERROR)) {
            debug->msg(QString::fromUtf8(line.mid(ERROR_SIZE)), "Helper", {Debug::LightRed});
        }
    }
}
