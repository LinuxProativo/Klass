/**
* @file StatusBar.hpp
 * @brief Header for the custom application status bar displaying update status and health metrics.
 */

#ifndef STATUSBAR_HPP
#define STATUSBAR_HPP

#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

/**
 * @enum StatusState
 * @brief Identifies the state displayed on the application status bar.
 */
enum class StatusState {
    UpToDate,
    Checking,
    DatabaseAvailable,
    PackagesAvailable
};

/**
 * @class StatusBar
 * @brief Lightweight status bar widget with an icon and status message.
 */
class StatusBar final : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);

    void setStatus(StatusState state, int count = 0) const;

private:
    QHBoxLayout *mainLayout{};
    QLabel *iconLabel{}, *textLabel{};
};

#endif
