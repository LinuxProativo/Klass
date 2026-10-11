/**
 * @file SettingsDialog.cpp
 * @brief Implementation of the SettingsDialog interface for application settings.
 */

#include <SettingsDialog.hpp>
#include <Utils.hpp>

/**
 * @brief Main Constructor. Sets up window bounds and groups configuration options across multiple tabs.
 * @param parent Pointer to the parent widget container.
 */
SettingsDialog::SettingsDialog(QWidget *parent) : Dialog(parent, Qt::ApplicationModal) {
    this->setWindowTitle(tr("Settings"));
    this->fixed();

    adminConfigManager = new AdminConfigManager();
    settingsManager = new SettingsManager(); // Aqui não vai this
    interfaceTab = new QWidget(this);

    startupGroup = new QGroupBox(tr("Startup Options"), interfaceTab);
    autostartCheckBox = new QCheckBox(tr("Start application automatically on system boot"), startupGroup);
    autostartCheckBox->setChecked(settingsManager->autostart());
    connect(autostartCheckBox, &QCheckBox::toggled, this, &SettingsDialog::onAutostartToggled);

    auto *hLine = new QFrame(startupGroup);
    hLine->setFrameShape(QFrame::HLine);
    hLine->setFrameShadow(QFrame::Sunken);

    startMaximizedCB = new QCheckBox(tr("Start program maximized"), startupGroup);
    startMaximizedCB->setChecked(settingsManager->startMaximized());
    connect(startMaximizedCB, &QCheckBox::toggled, this, &SettingsDialog::onStartMaximizedToggled);

    startMinimizedCB = new QCheckBox(tr("Start program minimized to tray"), startupGroup);
    startMinimizedCB->setChecked(settingsManager->startMinimized());
    connect(startMinimizedCB, &QCheckBox::toggled, this, &SettingsDialog::onStartMinimizedToggled);

    startupLayout = new QVBoxLayout(startupGroup);
    startupLayout->addWidget(autostartCheckBox);
    startupLayout->addWidget(hLine);
    startupLayout->addWidget(startMaximizedCB);
    startupLayout->addWidget(startMinimizedCB);

    updateOptionsGroup = new QGroupBox(tr("Update Options"), interfaceTab);
    confirmSelectUpdatesCB = new QCheckBox(tr("Show confirmation dialog when selecting all updates"), updateOptionsGroup);
    confirmSelectUpdatesCB->setChecked(settingsManager->confirmSelectAllUpdates());
    connect(confirmSelectUpdatesCB, &QCheckBox::toggled, this, &SettingsDialog::onConfirmSelectUpdatesToggled);

    officialUpdatesOnlyCB = new QCheckBox(tr("Select updates only from official repository"), updateOptionsGroup);
    officialUpdatesOnlyCB->setChecked(settingsManager->officialUpdatesOnly());
    connect(officialUpdatesOnlyCB, &QCheckBox::toggled, this, &SettingsDialog::onOfficialUpdatesOnlyToggled);

    updateOptionsLayout = new QVBoxLayout(updateOptionsGroup);
    updateOptionsLayout->addWidget(confirmSelectUpdatesCB);
    updateOptionsLayout->addWidget(officialUpdatesOnlyCB);

    repoGroup = new QGroupBox(tr("Repository Options"), interfaceTab);
    attachPatchesCB = new QCheckBox(tr("Attach 'patches' repository to official repository"), repoGroup);
    attachPatchesCB->setChecked(settingsManager->attachPatches());
    connect(attachPatchesCB, &QCheckBox::toggled, this, &SettingsDialog::onAttachPatchesToggled);

    attachTestingCB = new QCheckBox(tr("Attach 'testing' repository to official repository"), repoGroup);
    attachTestingCB->setChecked(settingsManager->attachTesting());
    connect(attachTestingCB, &QCheckBox::toggled, this, &SettingsDialog::onAttachTestingToggled);

    repoLayout = new QVBoxLayout(repoGroup);
    repoLayout->addWidget(attachPatchesCB);
    repoLayout->addWidget(attachTestingCB);

    interfaceLayout = new QVBoxLayout(interfaceTab);
    interfaceLayout->addWidget(startupGroup);
    interfaceLayout->addWidget(updateOptionsGroup);
    interfaceLayout->addWidget(repoGroup);
    interfaceLayout->addStretch();

    commandsTab = new QWidget(this);
    downloadCommandsGroup = new QGroupBox(tr("Download Options"), commandsTab);

    downloadMethodLabel = new QLabel(tr("Download Command:"), downloadCommandsGroup);
    downloadMethodCombo = new QComboBox(downloadCommandsGroup);
    downloadMethodCombo->addItem(tr("Native"));
    // downloadMethodCombo->addItem(QStringLiteral("wget")); // TODO Add support
    // downloadMethodCombo->addItem(QStringLiteral("aria2c")); // TODO Add support

    if (const int methodIdx = downloadMethodCombo->findText(adminConfigManager->downloadCommand()); methodIdx != -1)
        downloadMethodCombo->setCurrentIndex(methodIdx);

    extraParamsLabel = new QLabel(tr("Extra Parameters:"), downloadCommandsGroup);
    extraParamsEdit = new QLineEdit(downloadCommandsGroup);
    extraParamsEdit->setPlaceholderText(tr("e.g. -c --timeout=30"));
    extraParamsEdit->setText(adminConfigManager->downloadExtraParams());

    const bool isExternal = (downloadMethodCombo->currentIndex() != 0);
    extraParamsLabel->setEnabled(isExternal);
    extraParamsEdit->setEnabled(isExternal);

    connect(downloadMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SettingsDialog::onDownloadMethodChanged);
    connect(extraParamsEdit, &QLineEdit::editingFinished, this,
            [this] { onExtraParamsChanged(extraParamsEdit->text()); });

    downloadCommandsLayout = new QVBoxLayout(downloadCommandsGroup);
    downloadCommandsLayout->addWidget(downloadMethodLabel);
    downloadCommandsLayout->addWidget(downloadMethodCombo);
    downloadCommandsLayout->addSpacing(10);
    downloadCommandsLayout->addWidget(extraParamsLabel);
    downloadCommandsLayout->addWidget(extraParamsEdit);

    commandsLayout = new QVBoxLayout(commandsTab);
    commandsLayout->addWidget(downloadCommandsGroup);
    commandsLayout->addStretch();

    adminTab = new QWidget(this);
    verificationGroup = new QGroupBox(tr("Verifications"), adminTab);

    verifySignatureCB = new QCheckBox(tr("Enable signature verification"), verificationGroup);
    verifySignatureCB->setChecked(adminConfigManager->verifySignature());
    connect(verifySignatureCB, &QCheckBox::toggled, this, &SettingsDialog::onVerifySignatureToggled);

    verifyChecksumCB = new QCheckBox(tr("Enable checksum verification"), verificationGroup);
    verifyChecksumCB->setChecked(adminConfigManager->verifyChecksum());
    connect(verifyChecksumCB, &QCheckBox::toggled, this, &SettingsDialog::onVerifyChecksumToggled);

    preferredChecksumLabel = new QLabel(tr("Preferred Checksum:"), verificationGroup);
    checksumComboBox = new QComboBox(verificationGroup);
    checksumComboBox->addItem(QStringLiteral("MD5"));
    checksumComboBox->setCurrentText(adminConfigManager->preferredChecksum());

    preferredChecksumLabel->setEnabled(verifyChecksumCB->isChecked());
    checksumComboBox->setEnabled(verifyChecksumCB->isChecked());

    connect(checksumComboBox, &QComboBox::currentTextChanged, this, &SettingsDialog::onPreferredChecksumChanged);

    checksumSelectLayout = new QHBoxLayout();
    checksumSelectLayout->addWidget(preferredChecksumLabel);
    checksumSelectLayout->addWidget(checksumComboBox);
    checksumSelectLayout->addStretch();

    verificationLayout = new QVBoxLayout(verificationGroup);
    verificationLayout->addWidget(verifySignatureCB);
    verificationLayout->addWidget(verifyChecksumCB);
    verificationLayout->addLayout(checksumSelectLayout);

    postInstallGroup = new QGroupBox(tr("Post-Installation Tasks"), adminTab);
    postInstallList = new QListWidget(postInstallGroup);
    postInstallList->setSelectionMode(QAbstractItemView::NoSelection);

    addPostInstallTask(tr("Run ldconfig"), PostInstallTask::Ldconfig);
    addPostInstallTask(tr("Update MIME database"), PostInstallTask::UpdateMimeDatabase);
    addPostInstallTask(tr("Update man database (mandb)"), PostInstallTask::UpdateManDb);
    addPostInstallTask(tr("Update desktop database"), PostInstallTask::UpdateDesktopDatabase);
    addPostInstallTask(tr("Update GTK icon cache"), PostInstallTask::UpdateGtkIconCache);
    addPostInstallTask(tr("Compile GLib schemas"), PostInstallTask::CompileGlibSchemas);
    addPostInstallTask(tr("Update GRUB bootloader"), PostInstallTask::UpdateGrub);
    addPostInstallTask(tr("Update LILO bootloader"), PostInstallTask::UpdateLilo);
    addPostInstallTask(tr("Generate initrd for generic kernel"), PostInstallTask::GenerateInitrd);
    addPostInstallTask(tr("Reinstall VirtualBox kernel modules"), PostInstallTask::ReinstallVBoxModules);

    connect(postInstallList, &QListWidget::itemChanged, this, &SettingsDialog::onPostInstallItemChanged);

    postInstallLayout = new QVBoxLayout(postInstallGroup);
    postInstallLayout->addWidget(postInstallList);

    adminLayout = new QVBoxLayout(adminTab);
    adminLayout->addWidget(verificationGroup);
    adminLayout->addWidget(postInstallGroup);
    adminLayout->addStretch();

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(interfaceTab, tr("Interface"));
    tabWidget->addTab(commandsTab, tr("External Commands"));
    tabWidget->addTab(adminTab, tr("Administrative Actions"));

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
}

/**
 * @brief Helper to append a checkable maintenance task item to the post-installation list.
 * @param label Visual description of the maintenance action.
 * @param task The task enumeration identifier.
 */
void SettingsDialog::addPostInstallTask(const QString &label, const PostInstallTask task) const {
    auto *item = new QListWidgetItem(label, postInstallList); //NOLINT
    item->setData(Qt::UserRole, static_cast<int>(task));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(adminConfigManager->postInstallTask(task) ? Qt::Checked : Qt::Unchecked);
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
 * @brief Slot triggered when start maximized option is toggled.
 * @param checked True if enabled, false otherwise.
 */
void SettingsDialog::onStartMaximizedToggled(const bool checked) const {
    settingsManager->startMaximized(checked);
    if (checked && startMinimizedCB->isChecked())
        startMinimizedCB->setChecked(false);
}

/**
 * @brief Slot triggered when start minimized option is toggled.
 * @param checked True if enabled, false otherwise.
 */
void SettingsDialog::onStartMinimizedToggled(const bool checked) const {
    settingsManager->startMinimized(checked);
    if (checked && startMaximizedCB->isChecked())
        startMaximizedCB->setChecked(false);
}

/**
 * @brief Slot triggered when confirmation dialog before selecting all updates is toggled.
 * @param checked True if enabled, false otherwise.
 */
void SettingsDialog::onConfirmSelectUpdatesToggled(const bool checked) const {
    settingsManager->confirmSelectAllUpdates(checked);
}

/**
 * @brief Slot triggered when official updates only option is toggled.
 * @param checked True if enabled, false otherwise.
 */
void SettingsDialog::onOfficialUpdatesOnlyToggled(const bool checked) const {
    settingsManager->officialUpdatesOnly(checked);
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

/**
 * @brief Slot triggered when the download backend command changes.
 * @param index Index of the selected command.
 */
void SettingsDialog::onDownloadMethodChanged(const int index) {
    const bool isExternal = index != 0;
    extraParamsLabel->setEnabled(isExternal);
    extraParamsEdit->setEnabled(isExternal);
    if (!isExternal)
        adminConfigManager->setDownloadCommand(QStringLiteral("Native"));
    else
        adminConfigManager->setDownloadCommand(downloadMethodCombo->currentText());
    emit adminConfigSaveRequested(adminConfigManager->serializePayload());
}

/**
 * @brief Slot triggered when additional download parameters are modified.
 * @param params The raw arguments string.
 */
void SettingsDialog::onExtraParamsChanged(const QString &params) {
    adminConfigManager->setDownloadExtraParams(params.trimmed());
    emit adminConfigSaveRequested(adminConfigManager->serializePayload());
}

/**
 * @brief Slot triggered when signature verification is toggled.
 * @param checked True if enabled, false otherwise.
 */
void SettingsDialog::onVerifySignatureToggled(const bool checked) {
    adminConfigManager->setVerifySignature(checked);
    emit adminConfigSaveRequested(adminConfigManager->serializePayload());
}

/**
 * @brief Slot triggered when checksum verification is toggled.
 * @param checked True if enabled, false otherwise.
 */
void SettingsDialog::onVerifyChecksumToggled(const bool checked) {
    preferredChecksumLabel->setEnabled(checked);
    checksumComboBox->setEnabled(checked);
    adminConfigManager->setVerifyChecksum(checked);
    emit adminConfigSaveRequested(adminConfigManager->serializePayload());
}

/**
 * @brief Slot triggered when preferred checksum algorithm is changed.
 * @param checksum The selected checksum algorithm name.
 */
void SettingsDialog::onPreferredChecksumChanged(const QString &checksum) {
    adminConfigManager->setPreferredChecksum(checksum);
    emit adminConfigSaveRequested(adminConfigManager->serializePayload());
}

/**
 * @brief Slot triggered when any item in the post-installation tasks list changes its check state.
 * @param item The modified item in the list widget.
 */
void SettingsDialog::onPostInstallItemChanged(const QListWidgetItem *item) {
    if (!item) return;

    const auto task = static_cast<PostInstallTask>(item->data(Qt::UserRole).toInt());
    const bool isChecked = (item->checkState() == Qt::Checked);

    adminConfigManager->setPostInstallTask(task, isChecked);
    emit adminConfigSaveRequested(adminConfigManager->serializePayload());
}
