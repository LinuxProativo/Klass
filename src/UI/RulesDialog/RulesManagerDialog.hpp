/**
 * @file RulesManagerDialog.hpp
 * @brief Header for RulesManagerDialog.
 */

#ifndef RULESMANAGERDIALOG_HPP
#define RULESMANAGERDIALOG_HPP

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QVBoxLayout>

#include <Dialog.hpp>
#include <RulesManager.hpp>
#include <TableView.hpp>

/**
 * @class RulesManagerDialog
 * @brief Dialog window for managing package exclusion rules and cross-repository rules.
 */
class RulesManagerDialog : public Dialog {
    Q_OBJECT

public:
    explicit RulesManagerDialog(QWidget *parent = nullptr);

    void setRepositories(const QStringList &repos) const;

    void setPriorityRepositories(const QStringList &repos) const;

    void loadRules(const QList<RuleEntry> &rules) const;

    void appendRuleToTable(const RuleEntry &rule) const;

    void removeRulesFromTable(const QList<RuleEntry> &rules) const;

    void replaceRuleInTable(const RuleEntry &oldRule, const RuleEntry &newRule) const;

signals:
    void ruleAdded(const RuleEntry &rule);

    void ruleRemoved(const QList<RuleEntry> &rules);

    void ruleEdited(const RuleEntry &oldRule, const RuleEntry &newRule);

private:
    void onAddExceptionClicked();

    void onDeleteExceptionClicked();

    void onAddPriorityClicked();

    void onDeletePriorityClicked();

    void onExceptionRowDoubleClicked(const QModelIndex &index);

    void onPriorityRowDoubleClicked(const QModelIndex &index);

    void onEditExceptionClicked();

    void onEditPriorityClicked();

    void editExceptionRow(int row);

    void editPriorityRow(int row);

    [[nodiscard]] RuleEntry ruleFromExceptionRow(int row) const;

    [[nodiscard]] RuleEntry ruleFromPriorityRow(int row) const;

    QComboBox *repoCombo{}, *priorityRepoCombo{}, *typeCombo{};
    QHBoxLayout *excFormLayout{}, *excBottomLayout{}, *priFormLayout{}, *priBottomLayout{};
    QLabel *infoLabel{};
    QLineEdit *targetEdit{}, *priorityPackageEdit{};
    QPushButton *addExceptionBtn{}, *deleteExceptionBtn{}, *editExceptionBtn{};
    QPushButton *addPriorityBtn{}, *deletePriorityBtn{}, *editPriorityBtn{};
    QTabWidget *tabWidget{};
    QStandardItemModel *exceptionsModel{}, *prioritiesModel{};
    QVBoxLayout *mainLayout{}, *exceptionsLayout{}, *prioritiesLayout{};
    QWidget *exceptionsContainer{}, *prioritiesContainer{};
    QStringList m_repos{};

    TableView *exceptionsTable{}, *prioritiesTable{};
};

#endif
