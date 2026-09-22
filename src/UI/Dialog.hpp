/**
 * @file Dialog.hpp
 * @brief Header for Dialog.
 */

#ifndef DIALOG_HPP
#define DIALOG_HPP

#include <QDialog>

#include <SettingsManager.hpp>

/**
 * @class Dialog
 * @brief Base class for application dialogs.
 */
class Dialog : public QDialog {
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr, Qt::WindowModality modality = Qt::ApplicationModal);

    void fixed() { isfixed = true; }

    void maximum() { this->setMaximumSize(SettingsManager::defSize()); }

    void minimum() { this->setMinimumSize(SettingsManager::defSize()); }

private:
    bool isfixed{false};

protected:
    void changeEvent(QEvent *event) override;

    void showEvent(QShowEvent *event) override;
};

#endif
