/**
 * @file About.cpp
 * @brief Implementation of the About dialog window that displays application details and version info.
 */

#include <About.hpp>
#include <SlackwareDefines.hpp>
#include <Version.hpp>

/**
 * @brief Constructs an About dialog window with a specified parent widget.
 * @param parent Pointer to the parent widget of this dialog.
 */
About::About(QWidget *parent) : Dialog(parent, Qt::WindowModal) {
    this->setWindowTitle(tr("About"));
    this->setMinimumWidth(500);
    this->fixed();

    QFont titleFont = this->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 10);

    dp = new DefaultPath();
    const QPixmap iconPixmap(dp->defaultPath("klass.png"));

    iconLabel = new QLabel(this);
    iconLabel->setPixmap(iconPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);

    titleLabel = new QLabel(tr("Klass Package Manager"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFont(titleFont);

    titleFont.setPointSize(titleFont.pointSize() - 9);
    versionLabel = new QLabel(VERSION + tr(" BETA"), this); //TODO BETA VERSION
    versionLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setFont(titleFont);

    descLabel = new QLabel(this);
    descLabel->setWordWrap(true);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setText(
        tr("Klass is a Qt-based graphical interface for Slackware package management, "
            "intuitively organized by categories. Acting as a frontend for pkgbuild, "
            "the tool aims to simplify system maintenance and mitigate instability issues."));

    textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(10, 5, 10, 20);
    textLayout->addWidget(descLabel);

    closeButton = new QPushButton(tr("Close"), this);
    connect(closeButton, &QPushButton::clicked, this, &About::close);

    btnLicenseKlass = new QPushButton(tr("Klass License"), this);
    btnLicenseIcons = new QPushButton(tr("Material Icons by Google License"), this);

    connect(btnLicenseKlass, &QPushButton::clicked, this, [this] {
        (new LicenseView(tr("Klass License"), dp->defaultPath("LICENSE"), this))->show();
    });

    connect(btnLicenseIcons, &QPushButton::clicked, this, [this] {
        (new LicenseView(tr("Material Icons License"), dp->defaultPath("icons/LICENSE"), this))->show();
    });

    buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(btnLicenseKlass);
    buttonLayout->addWidget(btnLicenseIcons);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    layout = new QVBoxLayout(this);
    layout->addWidget(iconLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(versionLabel);
    layout->addLayout(textLayout);
    layout->addLayout(buttonLayout);
}
