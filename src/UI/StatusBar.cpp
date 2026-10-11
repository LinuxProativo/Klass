/**
 * @file StatusBar.cpp
 * @brief Implementation of the custom application status bar.
 */

#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QStyle>

#include <DefaultPath.hpp>
#include <StatusBar.hpp>

/**
 * @brief Constructs the status bar layout and initial state.
 * @param parent Parent widget.
 */
StatusBar::StatusBar(QWidget *parent) : QWidget(parent) {
    this->setFixedHeight(28);

    mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 2, 10, 2);
    mainLayout->setSpacing(8);

    iconLabel = new QLabel(this);
    iconLabel->setFixedSize(18, 18);
    iconLabel->setScaledContents(true);

    textLabel = new QLabel(this);

    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(textLabel);
    mainLayout->addStretch();

    setStatus(StatusState::UpToDate);
}

/**
 * @brief Updates the status bar icon and label according to current application state.
 * @param state The target status state.
 * @param count Number of pending packages when state is PackagesAvailable.
 */
void StatusBar::setStatus(const StatusState state, const int count) const {
    QPixmap iconPixmap;
    QString message;

    switch (state) {
        case StatusState::Checking: {
            const QString icoPath = DefaultPath::defaultPath("icons/update.svg");
            if (QFileInfo::exists(icoPath))
                iconPixmap = QIcon(icoPath).pixmap(18, 18);
            else
                iconPixmap = QApplication::style()->standardIcon(QStyle::SP_BrowserReload).pixmap(18, 18);
            message = tr("Checking for updates...");
            break;
        }
        case StatusState::DatabaseAvailable: {
            const QString icoPath = DefaultPath::defaultPath("icons/available.svg");
            if (QFileInfo::exists(icoPath))
                iconPixmap = QIcon(icoPath).pixmap(18, 18);
            else
                iconPixmap = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(18, 18);
            message = tr("Database update is available.");
            break;
        }
        case StatusState::PackagesAvailable: {
            const QString icoPath = DefaultPath::defaultPath("icons/up.svg");
            if (QFileInfo::exists(icoPath))
                iconPixmap = QIcon(icoPath).pixmap(18, 18);
            else
                iconPixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowUp).pixmap(18, 18);
            message = tr("%1 package(s) available for update.").arg(count);
            break;
        }
        case StatusState::UpToDate:
        default: {
            const QString okPath = DefaultPath::defaultPath("icons/ok.svg");
            if (QFileInfo::exists(okPath)) {
                iconPixmap = QIcon(okPath).pixmap(18, 18);
            } else {
                iconPixmap = QApplication::style()->standardIcon(QStyle::SP_DialogApplyButton).pixmap(18, 18);
            }
            message = tr("All packages are up to date.");
            break;
        }
    }

    iconLabel->setPixmap(iconPixmap);
    textLabel->setText(message);
}
