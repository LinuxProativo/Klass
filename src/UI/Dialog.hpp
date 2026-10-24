/**
 * @file Dialog.hpp
 * @brief Header for base Dialog class.
 */

#ifndef DIALOG_HPP
#define DIALOG_HPP

#include <QDialog>

#include <SettingsManager.hpp>

/**
 * @class Dialog
 * @brief Base class for application dialogs with automatic memory reclamation.
 */
class Dialog : public QDialog {
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr, Qt::WindowModality modality = Qt::ApplicationModal);

    ~Dialog() override;

    void fixed() { isfixed = true; }

    void maximum() { this->setMaximumSize(SettingsManager::defSize()); }

    void minimum() { this->setMinimumSize(SettingsManager::defSize()); }

protected:
    void changeEvent(QEvent *event) override;

    void showEvent(QShowEvent *event) override;

private:
    bool isfixed{false};
};

#endif
