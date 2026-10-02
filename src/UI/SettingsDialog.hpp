/**
 * @file SettingsDialog.hpp
 * @brief Header for config Settings.
 */

#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTabWidget>
#include <QVBoxLayout>

#include <AdminConfigManager.hpp>
#include <Dialog.hpp>
#include <SettingsManager.hpp>

/**
 * @class SettingsDialog
 * @brief Dialog window for managing application-wide settings, external commands, and administrative actions.
 */
class SettingsDialog : public Dialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

signals:
    void settingsChanged();

    void adminConfigSaveRequested(const QString &payload);

private:
    void onAutostartToggled(bool checked) const;

    void onAttachPatchesToggled(bool checked);

    void onAttachTestingToggled(bool checked);

    void onDownloadMethodChanged(int index);

    void onExtraParamsChanged(const QString &params);

    void onVerifySignatureToggled(bool checked);

    void onVerifyChecksumToggled(bool checked);

    void onPreferredChecksumChanged(const QString &checksum);

    void onPostInstallItemChanged(const QListWidgetItem *item);

    void addPostInstallTask(const QString &label, PostInstallTask task) const;

    QCheckBox *autostartCheckBox{}, *attachPatchesCB{}, *attachTestingCB{};
    QCheckBox *verifySignatureCB{}, *verifyChecksumCB{};
    QComboBox *downloadMethodCombo{}, *checksumComboBox{};
    QGroupBox *verificationGroup{}, *postInstallGroup{}, *startupGroup{}, *repoGroup{}, *downloadCommandsGroup{};
    QHBoxLayout *checksumSelectLayout{};
    QLabel *downloadMethodLabel{}, *extraParamsLabel{}, *preferredChecksumLabel{};
    QLineEdit *extraParamsEdit{};
    QListWidget *postInstallList{};
    QTabWidget *tabWidget{};
    QVBoxLayout *adminLayout{}, *verificationLayout{}, *postInstallLayout{}, *mainLayout{};
    QVBoxLayout *commandsLayout{}, *downloadCommandsLayout{}, *interfaceLayout{}, *startupLayout{}, *repoLayout{};
    QWidget *interfaceTab{}, *commandsTab{}, *adminTab{};

    SettingsManager *settingsManager{};
    AdminConfigManager *adminConfigManager{};
};

#endif
