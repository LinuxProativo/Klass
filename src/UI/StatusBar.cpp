/**
 * @file StatusBar.cpp
 * @brief Implementation of the custom application status bar.
 */

#include <QPainter>

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

    QFont font = this->font();
    font.setPointSizeF(font.pointSizeF() + 0.5);

    QFont boldFont = font;
    boldFont.setBold(true);

    iconLabel = new QLabel(container);
    iconLabel->setFixedSize(22, 22);
    iconLabel->setScaledContents(true);

    textLabel = new QLabel(container);
    textLabel->setFont(font);

    countLabel = new QLabel(container);
    countLabel->setFont(boldFont);

    suffixLabel = new QLabel(container);
    suffixLabel->setFont(font);

    primaryTextLayout = new QHBoxLayout();
    primaryTextLayout->setContentsMargins(0, 0, 0, 0);
    primaryTextLayout->setSpacing(0);
    primaryTextLayout->addWidget(textLabel);
    primaryTextLayout->addWidget(countLabel);
    primaryTextLayout->addWidget(suffixLabel);

    configContainer = new QWidget(container);
    configLayout = new QHBoxLayout(configContainer);
    configLayout->setContentsMargins(0, 0, 0, 0);
    configLayout->setSpacing(8);

    iconLabel2 = new QLabel(configContainer);
    iconLabel2->setFixedSize(22, 22);
    iconLabel2->setScaledContents(true);

    midTextLabel = new QLabel(configContainer);
    midTextLabel->setFont(font);

    countLabel2 = new QLabel(configContainer);
    countLabel2->setFont(boldFont);

    suffixLabel2 = new QLabel(configContainer);
    suffixLabel2->setFont(font);

    secondaryTextLayout = new QHBoxLayout();
    secondaryTextLayout->setContentsMargins(0, 0, 0, 0);
    secondaryTextLayout->setSpacing(0);
    secondaryTextLayout->addWidget(midTextLabel);
    secondaryTextLayout->addWidget(countLabel2);
    secondaryTextLayout->addWidget(suffixLabel2);

    configLayout->addWidget(iconLabel2);
    configLayout->addLayout(secondaryTextLayout);

    configContainer->setVisible(false);

    mainLayout->addWidget(iconLabel);
    mainLayout->addLayout(primaryTextLayout);
    mainLayout->addSpacing(8);
    mainLayout->addWidget(configContainer);
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
 * @param state The target status state to represent visually.
 * @param count Number of pending package updates (used in PackagesAvailable and PackagesAndNewConfigAvailable).
 * @param newConfigCount Number of pending .new files (used in NewConfigAvailable and PackagesAndNewConfigAvailable).
 */
void StatusBar::setStatus(const StatusState state, const int count, const int newConfigCount) {
    if (state == StatusState::Checking) {
        if (!spinnerTimer->isActive()) {
            const Icon::IconManager icon(22);
            baseCheckingPixmap = icon.getIcon(Icon::Id::Update).pixmap(22, 22);
            rotationAngle = 0;
            iconLabel->setPixmap(baseCheckingPixmap);
            spinnerTimer->start();
        }
        textLabel->setText(tr("Checking for updates") + QStringLiteral("..."));
        countLabel->clear();
        suffixLabel->clear();
        configContainer->setVisible(false);
        return;
    }

    if (spinnerTimer->isActive())
        spinnerTimer->stop();

    const Icon::IconManager icon(22);
    QPixmap iconPixmap;

    countLabel->clear();
    suffixLabel->clear();
    configContainer->setVisible(false);

    switch (state) {
        case StatusState::DatabaseAvailable: {
            iconPixmap = icon.getIcon(Icon::Id::Available, {}).pixmap(22, 22);
            textLabel->setText(tr("Database update is available."));
            break;
        }
        case StatusState::PackagesAvailable: {
            iconPixmap = icon.getIcon(Icon::Id::AvailableUpdate, {}).pixmap(22, 22);
            textLabel->setText(tr("Packages available for update: "));
            countLabel->setText(QString::number(count));
            suffixLabel->setText(QStringLiteral("."));
            break;
        }
        case StatusState::NewConfigAvailable: {
            iconPixmap = icon.getIcon(Icon::Id::New, COLOR_ORANGE).pixmap(22, 22);
            textLabel->setText(tr("Files .new pending for review: "));
            countLabel->setText(QString::number(newConfigCount));
            suffixLabel->setText(QStringLiteral("."));
            break;
        }
        case StatusState::PackagesAndNewConfigAvailable: {
            iconPixmap = icon.getIcon(Icon::Id::AvailableUpdate, {}).pixmap(22, 22);
            textLabel->setText(tr("Packages available for update: "));
            countLabel->setText(QString::number(count));
            suffixLabel->setText(QStringLiteral("."));

            iconLabel2->setPixmap(icon.getIcon(Icon::Id::New, COLOR_ORANGE).pixmap(22, 22));
            midTextLabel->setText(tr("Files .new pending for review: "));
            countLabel2->setText(QString::number(newConfigCount));
            suffixLabel2->setText(QStringLiteral("."));

            configContainer->setVisible(true);
            break;
        }
        case StatusState::UpToDate:
        default: {
            iconPixmap = icon.getIcon(Icon::Id::Ok, COLOR_GREEN).pixmap(22, 22);
            textLabel->setText(tr("All packages are up to date."));
            break;
        }
    }

    iconLabel->setPixmap(iconPixmap);
}
