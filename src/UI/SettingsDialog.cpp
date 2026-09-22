/**
 * @file SettingsDialog.cpp
 * @brief Implementation of the SettingsDialog interface for application settings.
 */

#include <SettingsDialog.hpp>
#include <Utils.hpp>

/**
 * @brief Main Constructor. Sets up window bounds and groups configuration options.
 * @param parent Pointer to the parent widget container.
 */
SettingsDialog::SettingsDialog(QWidget *parent) : Dialog(parent, Qt::ApplicationModal) {
    this->setWindowTitle(tr("Settings"));
    this->fixed();

    startupGroup = new QGroupBox(tr("Startup Options"), this);
    repoGroup = new QGroupBox(tr("Repository Options"), this);
    settingsManager = new SettingsManager(); // Aqui não vai this

    autostartCheckBox = new QCheckBox(tr("Start application automatically on system boot"), startupGroup);
    autostartCheckBox->setChecked(settingsManager->autostart());
    connect(autostartCheckBox, &QCheckBox::toggled, this, &SettingsDialog::onAutostartToggled);

    startupLayout = new QVBoxLayout(startupGroup);
    startupLayout->addWidget(autostartCheckBox);

    attachPatchesCB = new QCheckBox(tr("Attach 'patches' repository to official repository"), repoGroup);
    attachPatchesCB->setChecked(settingsManager->attachPatches());
    connect(attachPatchesCB, &QCheckBox::toggled, this, &SettingsDialog::onAttachPatchesToggled);

    attachTestingCB = new QCheckBox(tr("Attach 'testing' repository to official repository"), repoGroup);
    attachTestingCB->setChecked(settingsManager->attachTesting());
    connect(attachTestingCB, &QCheckBox::toggled, this, &SettingsDialog::onAttachTestingToggled);

    repoLayout = new QVBoxLayout(repoGroup);
    repoLayout->addWidget(attachPatchesCB);
    repoLayout->addWidget(attachTestingCB);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(startupGroup);
    mainLayout->addWidget(repoGroup);
}

/**
 * @brief Slot triggered immediately when the autostart option is toggled.
 * @param checked True if enabled, false otherwise.
 */
void SettingsDialog::onAutostartToggled(const bool checked) const {
    settingsManager->autostart(checked);
    Utils::setAutostart(checked);
}

/**
 * @brief Slot triggered immediately when attaching the patches repository is toggled.
 * @param checked True if enabled, false otherwise.
 */
void SettingsDialog::onAttachPatchesToggled(const bool checked) {
    settingsManager->attachPatches(checked);
    emit settingsChanged();
}

/**
 * @brief Slot triggered immediately when attaching the testing repository is toggled.
 * @param checked True if enabled, false otherwise.
 */
void SettingsDialog::onAttachTestingToggled(const bool checked) {
    settingsManager->attachTesting(checked);
    emit settingsChanged();
}
