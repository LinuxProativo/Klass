/**
 * @file CustomRepoDialog.cpp
 * @brief Implementation for adding custom mirror/repository URLs manually.
 */

#include <QMessageBox>

#include <CustomRepoDialog.hpp>

/**
 * @brief Constructs a CustomRepoDialog based on the target mode.
 * @param mode Mode::OfficialMirror or Mode::ThirdPartyRepo.
 * @param parent Parent widget.
 */
CustomRepoDialog::CustomRepoDialog(const Mode mode, QWidget *parent) : Dialog(parent), mode(mode) {
    this->setMinimumWidth(450);
    this->fixed();

    urlEdit = new QLineEdit(this);
    urlEdit->setPlaceholderText(tr("https://... or http://..."));

    formLayout = new QFormLayout();
    if (mode == Mode::OfficialMirror) {
        this->setWindowTitle(tr("Add Custom Official Mirror"));
        formLayout->addRow(tr("Mirror URL") + ":", urlEdit);
    } else {
        this->setWindowTitle(tr("Add Custom Repository"));
        nameEdit = new QLineEdit(this);
        nameEdit->setPlaceholderText(tr("Repository Name"));
        formLayout->addRow(tr("Repo Name") + ":", nameEdit);
        formLayout->addRow(tr("Repo URL") + ":", urlEdit);
    }

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CustomRepoDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonBox);
}

/**
 * @brief Validates inputs before accepting the dialog.
 */
void CustomRepoDialog::onAccept() {
    if (mode == Mode::ThirdPartyRepo && name().isEmpty()) {
        QMessageBox::warning(this, tr("Input Error"), tr("Repository name cannot be empty."));
        nameEdit->setFocus();
        return;
    }

    if (url().isEmpty()) {
        QMessageBox::warning(this, tr("Input Error"), tr("URL cannot be empty."));
        urlEdit->setFocus();
        return;
    }

    this->accept();
}
