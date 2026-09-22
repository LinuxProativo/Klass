/**
 * @file MirrorManager.cpp
 * @brief Provides a dialog for managing the official Slackware mirror and third-party repositories.
 */

#include <QHeaderView>

#include <CustomRepoDialog.hpp>
#include <MirrorManager.hpp>
#include <MirrorPickerDialog.hpp>
#include <RepoPlusPickerDialog.hpp>

/**
 * @brief Constructs the mirror manager dialog.
 * @param parent Parent widget.
 */
MirrorManager::MirrorManager(QWidget *parent) : Dialog(parent) {
    this->setWindowTitle(tr("Repository Manager"));
    this->setMinimumSize(768, 432);
    this->fixed();

    officialMirror = new QLineEdit(this);
    officialMirror->setReadOnly(true);
    officialMirror->setPlaceholderText(tr("No official mirror configured."));

    btnChange = new QPushButton(tr("Change") + "...", this);
    btnAddCustomOfficial = new QPushButton(tr("Add Custom"), this);

    btnAdd = new QPushButton(tr("Add") + "...", this);
    btnAddCustomThird = new QPushButton(tr("Add Custom"), this);
    btnRemove = new QPushButton(tr("Remove"), this);

    connect(btnChange, &QPushButton::clicked, this, &MirrorManager::onChangeClicked);
    connect(btnAddCustomOfficial, &QPushButton::clicked, this, &MirrorManager::onAddCustomOfficialClicked);

    connect(btnAdd, &QPushButton::clicked, this, &MirrorManager::onAddClicked);
    connect(btnAddCustomThird, &QPushButton::clicked, this, &MirrorManager::onAddCustomThirdClicked);
    connect(btnRemove, &QPushButton::clicked, this, &MirrorManager::onRemoveClicked);

    officialLayout = new QHBoxLayout();
    officialLayout->addWidget(officialMirror);
    officialLayout->addWidget(btnChange);
    officialLayout->addWidget(btnAddCustomOfficial);

    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({tr("Repository"), tr("URL")});

    repositoryList = new TableView(this);
    repositoryList->setModel(model);
    repositoryList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    buttonLayout = new QVBoxLayout();
    buttonLayout->addWidget(btnAdd);
    buttonLayout->addWidget(btnAddCustomThird);
    buttonLayout->addWidget(btnRemove);
    buttonLayout->addStretch();

    repositoryLayout = new QHBoxLayout();
    repositoryLayout->addWidget(repositoryList);
    repositoryLayout->addLayout(buttonLayout);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(new QLabel(tr("Official Slackware Mirror")));
    mainLayout->addLayout(officialLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(new QLabel(tr("Third-party Repositories")));
    mainLayout->addLayout(repositoryLayout);
    populate();
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
    for (const Mirrors::MirrorPlusEntry &entry: Mirrors::listActive()) {
        model->appendRow({new QStandardItem(entry.name), new QStandardItem(entry.url)});
    }
}

/**
 * @brief Opens a dialog to change the official or active repository mirror.
 */
void MirrorManager::onChangeClicked() {
    if (MirrorPickerDialog picker(this); picker.exec() == Accepted) {
        emit mirrorChangeRequested(picker.selectedMirror());
    }
}

/**
 * @brief Opens a custom dialog to enter an official mirror URL manually.
 */
void MirrorManager::onAddCustomOfficialClicked() {
    if (CustomRepoDialog dialog(Mode::OfficialMirror, this); dialog.exec() == Accepted) {
        emit mirrorChangeRequested(dialog.url());
    }
}

/**
 * @brief Opens a dialog to add or enable third-party (Slackpkg+) repositories.
 */
void MirrorManager::onAddClicked() {
    if (RepoPlusPickerDialog picker(this); picker.exec() == Accepted) {
        emit reposEnable(picker.selectedRepos());
    }
}

/**
 * @brief Opens a custom dialog to enter a third-party repository name and URL manually.
 */
void MirrorManager::onAddCustomThirdClicked() {
    if (CustomRepoDialog dialog(Mode::ThirdPartyRepo, this); dialog.exec() == Accepted) {
        emit reposEnable({{dialog.name(), dialog.url()}});
    }
}

/**
 * @brief Collects selected repositories from the list and requests their removal.
 */
void MirrorManager::onRemoveClicked() {
    const QModelIndexList selected = repositoryList->selectionModel()->selectedRows(0);
    if (selected.isEmpty()) return;

    QStringList names;
    for (const QModelIndex &idx: selected) {
        names << model->item(idx.row(), 0)->text();
    }

    emit reposRemoved(names);
}

/**
 * @brief Handles the event triggered when the dialog is shown.
 * @param event Pointer to the QShowEvent object containing event parameters.
 */
void MirrorManager::showEvent(QShowEvent *event) {
    populate();
    Dialog::showEvent(event);
}
