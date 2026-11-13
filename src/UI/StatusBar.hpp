/**
 * @file StatusBar.hpp
 * @brief Header for the custom application status bar displaying update status and health metrics.
 */

#ifndef STATUSBAR_HPP
#define STATUSBAR_HPP

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @enum StatusState
 * @brief Identifies the state displayed on the application status bar.
 */
enum class StatusState {
    UpToDate,
    Checking,
    DatabaseAvailable,
    PackagesAvailable,
    NewConfigAvailable,
    PackagesAndNewConfigAvailable
};

/**
 * @class StatusBar
 * @brief Lightweight status bar widget with dynamic icons and state indicators.
 */
class StatusBar final : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);

    void setStatus(StatusState state, int count = 0, int newConfigCount = 0);

private:
    QFrame *topLine{};
    QHBoxLayout *mainLayout{}, *primaryTextLayout{}, *configLayout{}, *secondaryTextLayout{};
    QLabel *iconLabel{}, *textLabel{}, *countLabel{}, *suffixLabel{};
    QLabel *iconLabel2{}, *midTextLabel{}, *countLabel2{}, *suffixLabel2{};
    QVBoxLayout *rootLayout{};
    QWidget *container{}, *configContainer{};

    QTimer *spinnerTimer{};
    QPixmap baseCheckingPixmap{};
    int rotationAngle{0};
};

#endif
