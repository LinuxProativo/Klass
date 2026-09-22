#ifndef INSTALLDIALOG_HPP
#define INSTALLDIALOG_HPP

#include <QLabel>
#include <QVBoxLayout>

#include <Dialog.hpp>

namespace InstallPackage {
    /**
     * @class Install
     * @brief Dialog window dedicated to the Slackware package installation process.
     */
    class Install : public Dialog {
        Q_OBJECT

    public:
        explicit Install(QWidget *parent = nullptr);

        void setPackage(const QString &packageName);

    private:
        QLabel *statusLabel;
    };
}

#endif
