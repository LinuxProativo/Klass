/**
 * @file CustomRepoDialog.hpp
 * @brief Header for adding custom mirror/repository URLs manually.
 */

#ifndef CUSTOMREPODIALOG_HPP
#define CUSTOMREPODIALOG_HPP

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

#include <Dialog.hpp>

/**
 * @enum Mode
 * @brief Specifies the operational mode for repository input.
 */
enum class Mode {
    OfficialMirror,
    ThirdPartyRepo
};

/**
 * @class CustomRepoDialog
 * @brief Dialog for manually entering mirror or third-party repository details.
 */
class CustomRepoDialog final : public Dialog {
    Q_OBJECT

public:
    explicit CustomRepoDialog(Mode mode, QWidget *parent = nullptr);

    [[nodiscard]] QString name() const { return nameEdit->text(); }

    [[nodiscard]] QString url() const { return urlEdit->text(); }

private:
    void onAccept();

    QDialogButtonBox *buttonBox{};
    QFormLayout *formLayout{};
    QHBoxLayout *buttonLayout{};
    QLineEdit *nameEdit{}, *urlEdit{};
    QPushButton *btnOk{}, *btnCancel{};
    QVBoxLayout *mainLayout{};

    Mode mode;
};

#endif
