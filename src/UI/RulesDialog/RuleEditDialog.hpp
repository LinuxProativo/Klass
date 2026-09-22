/**
 * @file RuleEditDialog.hpp
 * @brief Header for editing an existing exception or priority rule.
 */

#ifndef RULEEDITDIALOG_HPP
#define RULEEDITDIALOG_HPP

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

#include <Dialog.hpp>
#include <RulesManager.hpp>

/**
 * @enum RuleMode
 * @brief Specifies which kind of rule is being edited.
 */
enum class RuleMode {
    Exception,
    Priority
};

/**
 * @class RuleEditDialog
 * @brief Dialog for editing the fields of an existing exception or priority rule.
 */
class RuleEditDialog final : public Dialog {
    Q_OBJECT

public:
    explicit RuleEditDialog(RuleMode mode, const RuleEntry &rule, const QStringList &repos, QWidget *parent = nullptr);

    [[nodiscard]] RuleEntry updatedRule() const { return result; }

private:
    void onAccept();

    QComboBox *repoCombo{}, *typeCombo{};
    QDialogButtonBox *buttonBox{};
    QFormLayout *formLayout{};
    QHBoxLayout *buttonLayout{}, *repoScopeLayout{};
    QLineEdit *targetEdit{};
    QPushButton *btnOk{}, *btnCancel{};
    QVBoxLayout *mainLayout{};

    RuleMode mode{};
    RuleEntry result{};
};

#endif
