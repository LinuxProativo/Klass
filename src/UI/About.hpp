/**
 * @file About.hpp
 * @brief Header for About viewer.
 */

#ifndef ABOUT_HPP
#define ABOUT_HPP

#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include <DefaultPath.hpp>
#include <Dialog.hpp>

/**
 * @class About
 * @brief Displays application information such as version, authorship, and licensing details in a dedicated dialog.
 */
class About : public Dialog {
    Q_OBJECT

public:
    explicit About(QWidget *parent = nullptr);

private:
    QHBoxLayout *buttonLayout{};
    QLabel *iconLabel{}, *titleLabel{}, *versionLabel{}, *descLabel{};
    QPushButton *closeButton{}, *btnLicenseKlass{}, *btnLicenseIcons{};
    QVBoxLayout *layout{}, *textLayout{};

    DefaultPath *dp{};
};

/**
 * @class LicenseView
 * @brief Dialog window that displays a license agreement text file.
 */
class LicenseView : public Dialog {
    Q_OBJECT

public:
    /**
     * @brief Constructs a LicenseView with a given window title, file path to the license text, and parent widget.
     * @param title The text string to be displayed as the dialog window title.
     * @param filePath The system path to the text file containing the license agreement.
     * @param parent Pointer to the parent widget of this dialog.
     */
    explicit LicenseView(const QString &title, const QString &filePath, QWidget *parent)
        : Dialog(parent, Qt::WindowModal) {
        this->setWindowTitle(title);
        this->setMinimumSize(480, 270);
        (new QVBoxLayout(this))->addWidget(new QTextEdit(this));
        QFile file(filePath);
        QString text{tr("Could not load license file.")};
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            text = QTextStream(&file).readAll();
        const auto textEdit = qobject_cast<QTextEdit *>(this->layout()->itemAt(0)->widget());
        textEdit->setReadOnly(true);
        textEdit->setPlainText(text);
    }
};

#endif
