/**
 * @file Terminal.cpp
 * @brief Provides a lightweight terminal interface for displaying process output and capturing keyboard input.
 */

#include <QMessageBox>

#include <Terminal.hpp>

/**
 * @brief Constructs the terminal widget and initializes UI components.
 * @param parent Parent widget.
 */
Terminal::Terminal(QWidget *parent) : Dialog(parent) {
    this->setWindowTitle("Generic Terminal");
    this->minimum();

    terminalView = new TerminalView(this);
    terminalView->setReadOnly(true);
    terminalView->setUndoRedoEnabled(false);
    terminalView->setLineWrapMode(QTextEdit::WidgetWidth);

    terminalView->setStyleSheet(R"(
        QTextEdit {
            background-color: #0d1117;
            color: #c9d1d9;
            font-family: monospace;
            border: none;
        }
    )");

    btnAction = new QPushButton(tr("Cancel"), this);
    connect(btnAction, &QPushButton::clicked, this, &Terminal::close);

    buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnAction);

    layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->addWidget(terminalView);
    layout->addLayout(buttonLayout);
}

/**
 * @brief Writes output received from the backend process and ensures the cursor blink is active while data is flowing.
 * @param data Raw output data.
 */
void Terminal::writeOutput(const QByteArray &data) const {
    appendText(QString::fromUtf8(data));
    terminalView->setCursorBlinking(true);
}

/**
 * @brief Sets terminal state as finished or running.
 * @param v True if process finished successfully.
 */
void Terminal::setFinished(const bool v) {
    finished = v;
    btnAction->setText(finished ? tr("Done") : tr("Cancel"));
    terminalView->setCursorBlinking(!finished);
}

/**
 * @brief Resets terminal state and clears all content.
 */
void Terminal::resetTerminal() {
    this->setWindowTitle("Generic Terminal");

    inputBuffer.clear();
    terminalView->clear();
    terminalView->setCursorBlinking(false);
    finished = false;
    btnAction->setText(tr("Cancel"));
}

/**
 * @brief Requests user confirmation before closing the terminal if a command is still running.
 * @return True if the terminal can be closed; otherwise, false.
 */
bool Terminal::confirmIfNeeded() {
    if (finished)
        return true;

    const auto ret = QMessageBox::question(
        this,
        tr("Cancel running process"),
        tr("A command is still running. Do you really want to cancel it?"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (ret != QMessageBox::Yes)
        return false;

    if (operationRequest)
        emit cancelOperationRequest();
    if (taskRequest)
        emit cancelRequested();
    return true;
}

/**
 * @brief Handles keyboard input and forwards it to backend process.
 * @param event Keyboard event containing user input.
 */
void Terminal::keyPressEvent(QKeyEvent *event) {
    terminalView->setFocus();

    if (finished)
        return;

    QByteArray out;

    switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            out = inputBuffer.toUtf8() + "\n";
            appendText("\n"); // simula ENTER visual
            inputBuffer.clear();
            emit inputEntered(out);
            return;

        case Qt::Key_Backspace:
            if (!inputBuffer.isEmpty()) {
                inputBuffer.chop(1);
                terminalView->deleteLastChar();
            }
            return;

        case Qt::Key_C:
            if (event->modifiers() & Qt::ControlModifier) {
                if (operationRequest)
                    emit cancelOperationRequest();
                if (taskRequest)
                    emit cancelRequested();
                appendText("^C\n");
                inputBuffer.clear();
            }
            return;

        default:
            break;
    }

    if (const QString text = event->text(); !text.isEmpty()) {
        inputBuffer += text;
        appendText(text);
    }
}

/**
 * @brief Handles widget close event.
 * @param event Window close event handler.
 */
void Terminal::closeEvent(QCloseEvent *event) {
    if (!confirmIfNeeded()) {
        event->ignore();
        return;
    }

    resetTerminal();
    operationRequest = taskRequest = false;
    Dialog::closeEvent(event);
}
