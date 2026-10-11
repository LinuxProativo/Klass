/**
 * @file Terminal.hpp
 * @brief Header for terminal emulation.
 */

#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <QPushButton>
#include <QVBoxLayout>

#include <Dialog.hpp>
#include <TerminalView.hpp>

/**
 * @class Terminal
 * @brief Provides a lightweight terminal-like widget that displays process output.
 */
class Terminal final : public Dialog {
    Q_OBJECT

public:
    explicit Terminal(QWidget *parent = nullptr);

    void writeOutput(const QByteArray &data) const;

    void setFinished(bool v);

    void setOperationRequest(const bool b) { operationRequest = b; }

    void setTaskRequest(const bool b) { taskRequest = b; }

signals:
    void inputEntered(const QByteArray &data);

    void cancelRequested();

    void cancelOperationRequest();

protected:
    void keyPressEvent(QKeyEvent *event) override;

    void closeEvent(QCloseEvent *event) override;

private:
    void appendText(const QString &text) const  { terminalView->appendTerminalText(text); }

    void resetTerminal();

    bool confirmIfNeeded();

    QHBoxLayout *buttonLayout{};
    QPushButton *btnAction{};
    QVBoxLayout *layout{};

    TerminalView *terminalView{};

    QString inputBuffer{};
    bool finished{false}, operationRequest{false}, taskRequest{false};
};

#endif
