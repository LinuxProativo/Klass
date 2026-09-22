/**
 * @file ChangeLog.hpp
 * @brief Header for view changelogs.
 */

#ifndef CHANGELOG_HPP
#define CHANGELOG_HPP

#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include <Dialog.hpp>

/**
 * @class ChangeLog
 * @brief Provides a modal dialog for displaying the Slackware system ChangeLog.
 */
class ChangeLog : public Dialog {
    Q_OBJECT

public:
    explicit ChangeLog(QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    QTabWidget *tabs{};
    QVBoxLayout *layout{};

    bool alertTriggered{false};
};

#endif
