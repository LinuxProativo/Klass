/**
 * @file TerminalView.cpp
 * @brief Implementation of TerminalView, a QTextEdit that overlays a blinking block cursor.
 */

#include <QPainter>
#include <QScrollBar>

#include <TerminalView.hpp>

/**
 * @brief Constructs the terminal view and sets up the blink timer.
 * @param parent Parent widget.
 */
TerminalView::TerminalView(QWidget *parent) : QTextEdit(parent) {
    blinkTimer = new QTimer(this);
    blinkTimer->setInterval(530);

    connect(blinkTimer, &QTimer::timeout, this, [this] {
        cursorVisible = !cursorVisible;
        viewport()->update();
    });
}

/**
 * @brief Enables or disables the blinking cursor overlay.
 * @param enabled True to start blinking, false to stop and hide cursor.
 */
void TerminalView::setCursorBlinking(const bool enabled) {
    blinkEnabled = enabled;

    if (enabled) {
        cursorVisible = true;
        blinkTimer->start();
    } else {
        blinkTimer->stop();
        cursorVisible = false;
    }

    viewport()->update();
}

/**
 * @brief Runs action while preserving the current scroll position.
 * @param action The operation to run (e.g. a document layout query or a text insert).
 */
void TerminalView::withPreservedScroll(const std::function<void()> &action) const {
    QScrollBar *vBar = verticalScrollBar();
    const int savedScroll = vBar->value();
    const bool wasAtBottom = savedScroll >= vBar->maximum() - 2;

    action();

    if (wasAtBottom) {
        vBar->setValue(vBar->maximum());
    } else if (vBar->value() != savedScroll) {
        vBar->setValue(savedScroll);
    }
}

/**
 * @brief Appends text at the true end of the document using a standalone QTextCursor.
 * @param text The text to append.
 */
void TerminalView::appendTerminalText(const QString &text) const {
    withPreservedScroll([this, &text] {
        QTextCursor cursor(document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(text);
    });
}

/**
 * @brief Deletes the last character in the document, preserving the scroll position.
 */
void TerminalView::deleteLastChar() const {
    withPreservedScroll([this] {
        QTextCursor cursor(document());
        cursor.movePosition(QTextCursor::End);
        cursor.deletePreviousChar();
    });
}

/**
 * @brief Paints the widget and overlays a blinking block cursor at the end of the text content.
 * @param event Paint event.
 */
void TerminalView::paintEvent(QPaintEvent *event) {
    QTextEdit::paintEvent(event);

    if (!blinkEnabled || !cursorVisible)
        return;

    QRect r;
    withPreservedScroll([this, &r] {
        QTextCursor tc(document());
        tc.movePosition(QTextCursor::End);
        r = cursorRect(tc);
    });

    const int cursorWidth = fontMetrics().horizontalAdvance(QChar(' '));

    QPainter painter(viewport());
    painter.fillRect(r.x(), r.y(), cursorWidth, r.height(), QColor(0xc9d1d9));
}
