/**
 * @file SettingsDialog.hpp
 * @brief Header for config Settings.
 */

#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>

#include <Dialog.hpp>
#include <SettingsManager.hpp>

/**
 * @class SettingsDialog
 * @brief Dialog window for managing application-wide settings and repository attachments.
 */
class SettingsDialog : public Dialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

signals:
    void settingsChanged();

private:
    void onAutostartToggled(bool checked) const;

    void onAttachPatchesToggled(bool checked);

    void onAttachTestingToggled(bool checked);

    QCheckBox *autostartCheckBox{}, *attachPatchesCB{}, *attachTestingCB{};
    QGroupBox *startupGroup{}, *repoGroup{};
    QVBoxLayout *mainLayout{}, *startupLayout{}, *repoLayout{};

    SettingsManager *settingsManager{};
};

#endif
