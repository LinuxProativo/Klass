/**
 * @file RepoVerificationDialog.cpp
 * @brief Implementation of the repository signature and checksum verification configuration dialog.
 */

#include <RepoVerificationDialog.hpp>

/**
 * @brief Constructs the verification dialog for a specific repository.
 * @param repoName Name of the repository being configured.
 * @param parent Pointer to parent widget.
 */
RepoVerificationDialog::RepoVerificationDialog(const QString &repoName, QWidget *parent)
    : Dialog(parent, Qt::ApplicationModal), currentRepoName(repoName) {
    this->setAttribute(Qt::WA_DeleteOnClose, false);
    this->setWindowTitle(tr("Repository Verification & Security"));
    this->setMinimumWidth(500);
    this->fixed();

    repoLabel = new QLabel(this);
    repoLabel->setText(tr("Configuring verifications for") + QStringLiteral(": <b>%1</b>").arg(repoName));
    repoLabel->setWordWrap(true);

    verificationGroup = new QGroupBox(tr("Security & Integrity Options"), this);

    checkVerifySignature = new QCheckBox(tr("Verify package OpenPGP signature (.asc)"), verificationGroup);
    checkVerifyChecksumSignature = new QCheckBox(tr("Verify CHECKSUMS file signature (.asc)"), verificationGroup);
    checkVerifyChecksum = new QCheckBox(tr("Verify package checksum integrity"), verificationGroup);
    connect(checkVerifyChecksum, &QCheckBox::toggled, this, &RepoVerificationDialog::onChecksumToggled);

    checksumLabel = new QLabel(tr("Preferred Algorithm:"), verificationGroup);
    checksumCombo = new QComboBox(verificationGroup);
    checksumCombo->addItems({QStringLiteral("MD5")});
    checksumCombo->setCurrentText(QStringLiteral("MD5"));

    checksumLayout = new QHBoxLayout();
    checksumLayout->addWidget(checksumLabel);
    checksumLayout->addWidget(checksumCombo);
    checksumLayout->addStretch();

    groupLayout = new QVBoxLayout(verificationGroup);
    groupLayout->setSpacing(5);
    groupLayout->addWidget(checkVerifySignature);
    groupLayout->addWidget(checkVerifyChecksumSignature);
    groupLayout->addWidget(checkVerifyChecksum);
    groupLayout->addSpacing(5);
    groupLayout->addLayout(checksumLayout);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(repoLabel);
    mainLayout->addWidget(verificationGroup);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(buttonBox);
}

/**
 * @brief Toggles algorithm combobox availability based on checksum verification checkbox.
 * @param checked True if checksum verification is active.
 */
void RepoVerificationDialog::onChecksumToggled(const bool checked) const {
    checksumLabel->setEnabled(checked);
    checksumCombo->setEnabled(checked);
}