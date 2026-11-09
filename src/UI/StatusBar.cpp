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

    topLine = new QFrame(this);
    topLine->setFrameShape(QFrame::HLine);
    topLine->setFrameShadow(QFrame::Plain);

    QPalette linePal = topLine->palette();
    QColor line = linePal.color(QPalette::Text);
    line.setAlpha(100);
    linePal.setColor(QPalette::WindowText, line);
    topLine->setPalette(linePal);

    container = new QWidget(this);
    container->setAutoFillBackground(false);

    mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(10, 4, 10, 5);
    mainLayout->setSpacing(8);

    iconLabel = new QLabel(container);
    iconLabel->setFixedSize(22, 22);
    iconLabel->setScaledContents(true);

    QFont font = this->font();
    font.setPointSizeF(font.pointSizeF() + 0.5);

    textLabel = new QLabel(container);
    textLabel->setFont(font);

    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(textLabel);
    mainLayout->addStretch();

    rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(topLine);
    rootLayout->addWidget(container);

    spinnerTimer = new QTimer(this);
    spinnerTimer->setInterval(40);
    connect(spinnerTimer, &QTimer::timeout, this, [this] {
        rotationAngle = (rotationAngle + 15) % 360;

        QPixmap rotated(22, 22);
        rotated.fill(Qt::transparent);

        QPainter painter(&rotated);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.translate(11, 11);
        painter.rotate(rotationAngle);
        painter.translate(-11, -11);
        painter.drawPixmap(0, 0, baseCheckingPixmap);
        painter.end();

        iconLabel->setPixmap(rotated);
    });

    setStatus(StatusState::UpToDate);
}

/**
 * @brief Updates the status bar icon and label according to current application state.
 * @param state The target status state.
 * @param count Number of pending packages when state is PackagesAvailable.
 */
void StatusBar::setStatus(const StatusState state, const int count) {
    if (state == StatusState::Checking) {
        if (!spinnerTimer->isActive()) {
            const Icon::IconManager icon(22);
            baseCheckingPixmap = icon.getIcon(Icon::Id::Update).pixmap(22, 22);
            rotationAngle = 0;
            iconLabel->setPixmap(baseCheckingPixmap);
            spinnerTimer->start();
        }
        textLabel->setText(tr("Checking for updates") + QStringLiteral("..."));
        return;
    }

    if (spinnerTimer->isActive())
        spinnerTimer->stop();

    const Icon::IconManager icon(22);
    QPixmap iconPixmap;
    QString message;

    switch (state) {
        case StatusState::DatabaseAvailable: {
            iconPixmap = icon.getIcon(Icon::Id::Available, {}).pixmap(22, 22);
            message = tr("Database update is available.");
            break;
        }
        case StatusState::PackagesAvailable: {
            iconPixmap = icon.getIcon(Icon::Id::AvailableUpdate, {}).pixmap(22, 22);
            message = tr("Packages available for update") + QStringLiteral(": %1.").arg(count);
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
