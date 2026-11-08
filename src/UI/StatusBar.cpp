/**
 * @file StatusBar.cpp
 * @brief Implementation of the custom application status bar.
 */

#include <Icon.hpp>
#include <SlackwareDefines.hpp>
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
    QColor line = linePal.color(QPalette::Text);
    line.setAlpha(100);
    linePal.setColor(QPalette::WindowText, line);
    topLine->setPalette(linePal);

    const auto container = new QWidget(this);
    container->setAutoFillBackground(false);

    mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(10, 3, 5, 3);
    mainLayout->setSpacing(8);

    iconLabel = new QLabel(container);
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
    const Icon::IconManager icon(22);
    QPixmap iconPixmap;
    QString message;

    switch (state) {
        case StatusState::Checking: {
            iconPixmap = icon.getIcon(Icon::Id::Update).pixmap(22, 22);
            message = tr("Checking for updates...");
            break;
        }
        case StatusState::DatabaseAvailable: {
            iconPixmap = icon.getIcon(Icon::Id::Available, {}).pixmap(22, 22);
            message = tr("Database update is available.");
            break;
        }
        case StatusState::PackagesAvailable: {
            iconPixmap = icon.getIcon(Icon::Id::Up).pixmap(22, 22);
            message = QStringLiteral("%1 ").arg(count) + tr("package(s) available for update.");
            break;
        }
        case StatusState::UpToDate:
        default: {
            iconPixmap = icon.getIcon(Icon::Id::Ok, COLOR_GREEN).pixmap(22, 22);
            message = tr("All packages are up to date.");
            break;
        }
    }

    iconLabel->setPixmap(iconPixmap);
    textLabel->setText(message);
}
