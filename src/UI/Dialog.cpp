/**
 * @file Dialog.cpp
 * @brief Custom dialog that restricts interaction with the main window while active.
 */

#include <Dialog.hpp>

/**
 * @brief Constructs the custom dialog.
 * @param parent Pointer to the parent widget.
 * @param modality Defines the dialog's modality (Qt::NonModal, Qt::WindowModal, or Qt::ApplicationModal).
 *   - Qt::ApplicationModal: blocks all windows.
 *   - Qt::WindowModal: blocks only the parent.
 *   - Qt::NonModal: allows interaction with all windows.
 */
Dialog::Dialog(QWidget *parent, const Qt::WindowModality modality) : QDialog(parent) {
    this->setAttribute(Qt::WA_StaticContents);
    this->setWindowModality(modality);
    this->setWindowFlags(Qt::Dialog);
}

/**
 * @brief Intercepts window state changes to prevent minimization.
 * @param event The QEvent instance.
 */
void Dialog::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange && this->windowModality() != Qt::NonModal) {
        if (this->windowState() & Qt::WindowMinimized) {
            this->setWindowState(Qt::WindowNoState);
        }
    }
    QDialog::changeEvent(event);
}

/**
 * @brief Handles the window show event to optionally lock the dialog size.
 * @param event The showEvent instance.
 */
void Dialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (isfixed)
        this->setFixedSize(this->size());
}
