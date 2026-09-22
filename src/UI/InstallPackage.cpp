/**
 * Manages the package installation process, providing a modal interface
 * to display status updates during the operation.
 */

#include <InstallPackage.hpp>

namespace InstallPackage {
    /**
     * @brief Constructs the installation dialog.
     * @param parent - Pointer to the parent widget.
     */
    Install::Install(QWidget *parent) : Dialog(parent, Qt::NonModal) {
        this->setWindowTitle(tr("Install Package"));
        this->fixed();
        this->maximum();

        auto *layout = new QVBoxLayout(this);
        statusLabel = new QLabel("Preparing installation...", this);
        layout->addWidget(statusLabel);
    }

    /**
     * @brief Updates the dialog status to reflect the current package being installed.
     * @param packageName - The name of the package currently being processed.
     */
    void Install::setPackage(const QString &packageName) {
        statusLabel->setText(QString("Installing: %1").arg(packageName));
    }
}
