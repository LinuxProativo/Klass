/**
 * @file RepoPlusPickerDialog.cpp
 * @brief Dialog for adding third-party repositories.
 */

#include <QHeaderView>
#include <QPushButton>

#include <RepoPlusPickerDialog.hpp>

/**
 * @brief Constructs the repo picker dialog.
 * @param parent Parent widget.
 */
RepoPlusPickerDialog::RepoPlusPickerDialog(QWidget *parent) : QDialog(parent) {
    this->setWindowTitle(tr("Add Repository"));
    this->setMinimumSize(672, 378);

    infoLabel = new QLabel(tr("Select Available Third-Party Repositories."), this);
    infoLabel->setWordWrap(true);

    model = new QStandardItemModel(0, 2, this);
    model->setHorizontalHeaderLabels({tr("Repository"), tr("URL")});

    repoTable = new TableView(this);
    repoTable->setModel(model);
    repoTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    repoTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RepoPlusPickerDialog::applySelection);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(repoTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection &selected) {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!selected.isEmpty());
    });

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(infoLabel);
    mainLayout->addWidget(repoTable);
    mainLayout->addWidget(buttonBox);

    populate();
}

/**
 * @brief Populates the table with compatible repositories not yet active.
 */
void RepoPlusPickerDialog::populate() const {
    const QList<Mirrors::MirrorPlusEntry> active = Mirrors::listActive();
    QSet<QPair<QString, QString>> activeEntries;
    for (const Mirrors::MirrorPlusEntry &e: active)
        activeEntries.insert({e.name, e.url});

    model->setRowCount(0);
    for (const auto &[name, url]: Mirrors::catalogue()) {
        if (activeEntries.contains({name, url}))
            continue;

        model->appendRow({new QStandardItem(name), new QStandardItem(url)});
    }

    if (model->rowCount() == 0)
        infoLabel->setText(tr("Repository List is Empty."));
}

/**
 * @brief Stores selected repository entries and closes the dialog with Accepted.
 */
void RepoPlusPickerDialog::applySelection() {
    const QModelIndexList selected = repoTable->selectionModel()->selectedRows(0);
    if (selected.isEmpty()) {
        reject();
        return;
    }

    selectedEntries.clear();
    for (const QModelIndex &i: selected)
        selectedEntries.append({model->item(i.row(), 0)->text(), model->item(i.row(), 1)->text()});
    accept();
}
