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
    this->setAutoFillBackground(true);

    QPalette pal = this->palette();
    pal.setColor(QPalette::Window, pal.color(QPalette::Base));
    setPalette(pal);

    const auto topLine = new QFrame(this);
    topLine->setFrameShape(QFrame::HLine);
    topLine->setFrameShadow(QFrame::Plain);

    QPalette linePal = topLine->palette();
    linePal.setColor(QPalette::WindowText, this->palette().color(QPalette::Mid));
    topLine->setPalette(linePal);

    const auto container = new QWidget(this);
    container->setAutoFillBackground(false);

    mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(5, 5, 5, 6);
    mainLayout->setSpacing(8);

    iconLabel = new QLabel(container);
    iconLabel->setFixedSize(18, 18);
    iconLabel->setScaledContents(true);

    textLabel = new QLabel(container);

    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(textLabel);
    mainLayout->addStretch();

    const auto rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(topLine);
    rootLayout->addWidget(container);

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
            if (const QString icoPath = DefaultPath::defaultPath("icons/update.svg"); QFileInfo::exists(icoPath))
                iconPixmap = QIcon(icoPath).pixmap(18, 18);
            else
                iconPixmap = QApplication::style()->standardIcon(QStyle::SP_BrowserReload).pixmap(18, 18);
            message = tr("Checking for updates...");
            break;
        }
        case StatusState::DatabaseAvailable: {
            if (const QString icoPath = DefaultPath::defaultPath("icons/available.svg"); QFileInfo::exists(icoPath))
                iconPixmap = QIcon(icoPath).pixmap(18, 18);
            else
                iconPixmap = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(18, 18);
            message = tr("Database update is available.");
            break;
        }
        case StatusState::PackagesAvailable: {
            if (const QString icoPath = DefaultPath::defaultPath("icons/up.svg"); QFileInfo::exists(icoPath))
                iconPixmap = QIcon(icoPath).pixmap(18, 18);
            else
                iconPixmap = QApplication::style()->standardIcon(QStyle::SP_ArrowUp).pixmap(18, 18);
            message = "%1 " + tr("package(s) available for update.").arg(count);
            break;
        }
        case StatusState::UpToDate:
        default: {
            if (const QString okPath = DefaultPath::defaultPath("icons/ok.svg"); QFileInfo::exists(okPath)) {
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
