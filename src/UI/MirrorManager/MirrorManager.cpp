/**
 * @file MirrorManager.cpp
 * @brief Provides a dialog for managing the official Slackware mirror and third-party repositories.
 */

#include <QHeaderView>

#include <CustomRepoDialog.hpp>
#include <MirrorManager.hpp>
#include <MirrorPickerDialog.hpp>
#include <RepoPlusPickerDialog.hpp>
#include <RepoVerificationDialog.hpp>

/**
 * @brief Constructs the mirror manager dialog.
 * @param parent Parent widget.
 */
MirrorManager::MirrorManager(QWidget *parent) : Dialog(parent) {
    this->setWindowTitle(tr("Repository Manager"));
    this->setMinimumSize(800, 450);
    this->fixed();

    officialMirror = new QLineEdit(this);
    officialMirror->setReadOnly(true);
    officialMirror->setPlaceholderText(tr("No official mirror configured."));

    btnChange = new QPushButton(tr("Change") + "...", this);
    btnAddCustomOfficial = new QPushButton(tr("Add Custom"), this);

    connect(btnChange, &QPushButton::clicked, this, &MirrorManager::onChangeClicked);
    connect(btnAddCustomOfficial, &QPushButton::clicked, this, &MirrorManager::onAddCustomOfficialClicked);

    officialLayout = new QHBoxLayout();
    officialLayout->addWidget(officialMirror);
    officialLayout->addWidget(btnChange);
    officialLayout->addWidget(btnAddCustomOfficial);

    checkPasture = new QCheckBox(tr("Enable Pasture"), this);
    checkTesting = new QCheckBox(tr("Enable Testing"), this);
    checkExtra = new QCheckBox(tr("Enable Extra"), this);

    connect(checkPasture, &QCheckBox::toggled, this, [this] {
        emit officialBranchesChanged({checkPasture->isChecked(), checkTesting->isChecked(), checkExtra->isChecked()});
        pending_update = true;
    });
    connect(checkTesting, &QCheckBox::toggled, this, [this] {
        emit officialBranchesChanged({checkPasture->isChecked(), checkTesting->isChecked(), checkExtra->isChecked()});
        pending_update = true;
    });
    connect(checkExtra, &QCheckBox::toggled, this, [this] {
        emit officialBranchesChanged({checkPasture->isChecked(), checkTesting->isChecked(), checkExtra->isChecked()});
        pending_update = true;
    });

    officialBranchesLayout = new QHBoxLayout();
    officialBranchesLayout->addWidget(checkPasture);
    officialBranchesLayout->addWidget(checkTesting);
    officialBranchesLayout->addWidget(checkExtra);
    officialBranchesLayout->addStretch();

    btnAdd = new QPushButton(tr("Add") + "...", this);
    btnAddCustomThird = new QPushButton(tr("Add Custom"), this);
    btnRemove = new QPushButton(tr("Remove"), this);
    btnVerification = new QPushButton(tr("Verifications") + "...", this);

    btnRemove->setEnabled(false);
    btnVerification->setEnabled(false);

    connect(btnAdd, &QPushButton::clicked, this, &MirrorManager::onAddClicked);
    connect(btnAddCustomThird, &QPushButton::clicked, this, &MirrorManager::onAddCustomThirdClicked);
    connect(btnRemove, &QPushButton::clicked, this, &MirrorManager::onRemoveClicked);
    connect(btnVerification, &QPushButton::clicked, this, &MirrorManager::onVerificationClicked);

    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({tr("Repository"), tr("URL")});

    repositoryList = new TableView(this);
    repositoryList->setModel(model);
    repositoryList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    connect(repositoryList->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MirrorManager::updateSelectionButtons);

    buttonLayout = new QVBoxLayout();
    buttonLayout->addWidget(btnAdd);
    buttonLayout->addWidget(btnAddCustomThird);
    buttonLayout->addWidget(btnRemove);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnVerification);

    repositoryLayout = new QHBoxLayout();
    repositoryLayout->addWidget(repositoryList);
    repositoryLayout->addLayout(buttonLayout);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->addWidget(new QLabel(tr("Official Slackware Mirror")));
    mainLayout->addLayout(officialLayout);
    mainLayout->addLayout(officialBranchesLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(new QLabel(tr("Third-party Repositories")));
    mainLayout->addLayout(repositoryLayout);
    populate();
}

/**
 * @brief Updates the enabled state of action buttons based on table selection.
 */
void MirrorManager::updateSelectionButtons() const {
    const bool hasSelection = !repositoryList->selectionModel()->selectedRows().isEmpty();
    btnRemove->setEnabled(hasSelection);
    btnVerification->setEnabled(hasSelection);
}

/**
 * @brief Updates the model by adding or replacing third-party mirror entries.
 * @param entries List of third-party mirror entries containing names and URLs.
 */
void MirrorManager::thirdMirrorUpdate(const QList<Mirrors::MirrorPlusEntry> &entries) {
    for (const auto &[name, url]: entries) {
        if (const QList<QStandardItem *> found = model->findItems(name, Qt::MatchExactly, 0); !found.isEmpty()) {
            model->setItem(found.first()->row(), 1, new QStandardItem(url));
        } else {
            model->appendRow({new QStandardItem(name), new QStandardItem(url)});
        }
    }
    pending_update = true;
}

/**
 * @brief Removes third-party mirrors from the model by their names.
 * @param names List of mirror names to be found and removed.
 */
void MirrorManager::thirdMirrorUpdate(const QStringList &names) {
    for (const QString &name: names) {
        for (const QList<QStandardItem *> i = model->findItems(name, Qt::MatchExactly, 0);
             const QStandardItem *item: i)
            model->removeRow(item->row());
    }
    pending_update = true;
}

/**
 * @brief Populates the dialog with the current mirror configuration.
 */
void MirrorManager::populate() const {
    const auto [url, country] = Mirrors::activeOne();
    officialMirror->setText(url);

    model->setRowCount(0);
    for (const Mirrors::MirrorPlusEntry &entry: Mirrors::listActive())
        model->appendRow({new QStandardItem(entry.name), new QStandardItem(entry.url)});

    const auto branches = Mirrors::loadOfficialBranches();
    checkPasture->setChecked(branches.pasture);
    checkTesting->setChecked(branches.testing);
    checkExtra->setChecked(branches.extra);
}

/**
 * @brief Opens a dialog to change the official or active repository mirror.
 */
void MirrorManager::onChangeClicked() {
    if (MirrorPickerDialog picker(this); picker.exec() == Accepted)
        emit mirrorChangeRequested(picker.selectedMirror());
}

/**
 * @brief Opens a custom dialog to enter an official mirror URL manually.
 */
void MirrorManager::onAddCustomOfficialClicked() {
    if (CustomRepoDialog dialog(Mode::OfficialMirror, this); dialog.exec() == Accepted)
        emit mirrorChangeRequested(dialog.url());
}

/**
 * @brief Opens a dialog to add or enable third-party (Slackpkg+) repositories.
 */
void MirrorManager::onAddClicked() {
    if (RepoPlusPickerDialog picker(this); picker.exec() == Accepted)
        emit reposEnable(picker.selectedRepos());
}

/**
 * @brief Opens a custom dialog to enter a third-party repository name and URL manually.
 */
void MirrorManager::onAddCustomThirdClicked() {
    if (CustomRepoDialog dialog(Mode::ThirdPartyRepo, this); dialog.exec() == Accepted)
        emit reposEnable({{dialog.name(), dialog.url()}});
}

/**
 * @brief Collects selected repositories from the list and requests their removal.
 */
void MirrorManager::onRemoveClicked() {
    const QModelIndexList selected = repositoryList->selectionModel()->selectedRows(0);
    if (selected.isEmpty()) return;

    QStringList names;
    for (const QModelIndex &idx: selected)
        names << model->item(idx.row(), 0)->text();

    emit reposRemoved(names);
}

/**
 * @brief Opens the verification settings dialog for the currently selected third-party repository.
 */
void MirrorManager::onVerificationClicked() {
    const QModelIndexList selected = repositoryList->selectionModel()->selectedRows(0);
    if (selected.isEmpty()) return;

    const QString repoName = model->item(selected.first().row(), 0)->text();
    RepoVerificationDialog dialog(repoName, this);

    const auto currentVerif = RepoVerificationManager::getVerification(repoName);
    dialog.setVerifySignature(currentVerif.verifySig);
    dialog.setVerifyChecksumSignature(currentVerif.verifyChecksumSig);
    dialog.setVerifyChecksum(currentVerif.verifyChecksum);
    dialog.setPreferredChecksum(currentVerif.preferredChecksum);

    if (dialog.exec() == Accepted) {
        RepoVerificationEntry entry;
        entry.repo = repoName;
        entry.verifySig = dialog.verifySignature();
        entry.verifyChecksumSig = dialog.verifyChecksumSignature();
        entry.verifyChecksum = dialog.verifyChecksum();
        entry.preferredChecksum = dialog.preferredChecksum();

        emit repoVerificationChanged(entry);
    }
}

/**
 * @brief Handles the event triggered when the dialog is shown.
 * @param event Pointer to the QShowEvent object containing event parameters.
 */
void MirrorManager::showEvent(QShowEvent *event) {
    populate();
    updateSelectionButtons();
    Dialog::showEvent(event);
}

/**
 * @brief Emits dialogClosed signal when the dialog is hidden or closed.
 * @param event Pointer to the QHideEvent object.
 */
void MirrorManager::hideEvent(QHideEvent *event) {
    Dialog::hideEvent(event);
    emit dialogClosed();
}
