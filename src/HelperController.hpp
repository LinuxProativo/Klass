/**
 * @file HelperController.hpp
 * @brief Header for privileged helper process control.
 */

#ifndef HELPERCONTROLLER_HPP
#define HELPERCONTROLLER_HPP

#include <QObject>
#include <QLocalSocket>

#include <Debug.hpp>
#include <Process.hpp>
#include <Terminal.hpp>

enum class PendingAction {
    None,
    Update,
    SetMirror,
    EnableRepo,
    DisableRepo,
    AddRule,
    DeleteRule,
    EditRule,
    ProcessTransaction
};

/**
 * @class HelperController
 * @brief Controls the privileged helper process and all communication with it.
 */
class HelperController final : public QObject {
    Q_OBJECT

public:
    explicit HelperController(Terminal *dialog, QObject *parent = nullptr);

    void setupHelper(PendingAction pending);

    [[nodiscard]] QLocalSocket *socket() const { return helperSocket; }

    [[nodiscard]] PendingAction pendingAct() const { return pendingAction; }

    void setPendingAct(const PendingAction act) { pendingAction = act; }

    void setHelperShutdownRequested() { helperShutdownRequested = true; }

    void onHelperConnected();

signals:
    void helperReady();

    void updateFinished();

    void updateRules();

    void transactionFinished();

private:
    void onHelperSocketError(QLocalSocket::LocalSocketError error);

    void onFinished(int exitCode, QProcess::ExitStatus status);

    void onHelperReadyRead();

    QLocalSocket *helperSocket{};

    Debug::Debug *debug{};
    Process *proc{};
    Terminal *terminalDialog{};

    PendingAction pendingAction{PendingAction::None};

    int countConnect{0};
    bool helperShutdownRequested{false};
};

#endif
