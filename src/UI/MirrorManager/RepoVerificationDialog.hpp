/**
 * @file RepoVerificationDialog.hpp
 * @brief Header for configuring repository signature and checksum verifications.
 */

#ifndef REPOVERIFICATIONDIALOG_HPP
#define REPOVERIFICATIONDIALOG_HPP

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <Dialog.hpp>

/**
 * @class RepoVerificationDialog
 * @brief Modal dialog to configure signature and checksum verification rules for a repository.
 */
class RepoVerificationDialog final : public Dialog {
    Q_OBJECT

public:
    explicit RepoVerificationDialog(const QString &repoName, QWidget *parent = nullptr);

    [[nodiscard]] bool verifySignature() const { return checkVerifySignature->isChecked(); }

    [[nodiscard]] bool verifyChecksumSignature() const { return checkVerifyChecksumSignature->isChecked(); }

    [[nodiscard]] bool verifyChecksum() const { return checkVerifyChecksum->isChecked(); }

    [[nodiscard]] QString preferredChecksum() const { return checksumCombo->currentText(); }

    void setVerifySignature(const bool enabled) const { checkVerifySignature->setChecked(enabled); }

    void setVerifyChecksumSignature(const bool enabled) const { checkVerifyChecksumSignature->setChecked(enabled); }

    void setVerifyChecksum(const bool enabled) const { checkVerifyChecksum->setChecked(enabled); }

    void setPreferredChecksum(const QString &algorithm) const { checksumCombo->setCurrentText(algorithm); }

private:
    void onChecksumToggled(bool checked) const;

    QLabel *repoLabel{}, *checksumLabel{};
    QCheckBox *checkVerifySignature{}, *checkVerifyChecksumSignature{}, *checkVerifyChecksum{};
    QComboBox *checksumCombo{};
    QGroupBox *verificationGroup{};
    QDialogButtonBox *buttonBox{};
    QHBoxLayout *checksumLayout{};
    QVBoxLayout *groupLayout{}, *mainLayout{};
    QString currentRepoName{};
};

#endif
