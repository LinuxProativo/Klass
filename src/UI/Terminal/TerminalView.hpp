/**
 * @file TerminalView.hpp
 * @brief Header for TerminalView.
 */

#ifndef TERMINALVIEW_HPP
#define TERMINALVIEW_HPP

#include <functional>

#include <QTextEdit>
#include <QTimer>
#include <QPaintEvent>

/**
 * @class TerminalView
 * @brief A QTextEdit subclass that renders a blinking block cursor overlay at the end of the content.
 */
class TerminalView final : public QTextEdit {
    Q_OBJECT

public:
    explicit TerminalView(QWidget *parent = nullptr);

    void setCursorBlinking(bool enabled);

    void appendTerminalText(const QString &text) const;

    void deleteLastChar() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void withPreservedScroll(const std::function<void()> &action) const;

    QTimer *blinkTimer{};
    bool cursorVisible{false}, blinkEnabled{false};
};

#endif
