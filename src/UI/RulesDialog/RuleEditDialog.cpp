/**
 * @file RuleEditDialog.cpp
 * @brief Implementation of RuleEditDialog.
 */

#include <QLabel>
#include <QMessageBox>

#include <RuleEditDialog.hpp>
#include <RuleUtils.hpp>
#include <SlackwareDefines.hpp>

/**
 * @brief Builds the edit form for the given rule, adapting fields to @p mode.
 * @param mode Whether the rule being edited is an Exception or a Priority.
 * @param rule The rule's current values.
 * @param repos Available repository identifiers for the repo combo box.
 * @param parent Parent widget.
 */
RuleEditDialog::RuleEditDialog(const RuleMode mode, const RuleEntry &rule, const QStringList &repos, QWidget *parent)
    : Dialog(parent, Qt::ApplicationModal), mode(mode), result(rule) {
    this->setWindowTitle(mode == RuleMode::Exception ? tr("Edit Exception") : tr("Edit Priority"));
    this->setMinimumWidth(450);
    this->fixed();

    mainLayout = new QVBoxLayout(this);
    formLayout = new QFormLayout();

    repoCombo = new QComboBox(this);
    if (mode == RuleMode::Exception)
        repoCombo->addItem(tr("Any Repository"), QStringLiteral("all"));

    for (const QString &repo: repos)
        repoCombo->addItem(RuleUtils::displayRepositoryName(repo), repo);
    repoCombo->addItem(tr("Others"), SLACK_OTHERS);
    if (const int idx = repoCombo->findData(rule.repo); idx != -1)
        repoCombo->setCurrentIndex(idx);

    repoScopeLayout = new QHBoxLayout();
    repoScopeLayout->addWidget(repoCombo, 1);

    if (mode == RuleMode::Exception) {
        typeCombo = new QComboBox(this);
        typeCombo->addItem(tr("By Package"), QStringLiteral("package"));
        typeCombo->addItem(tr("By Category"), QStringLiteral("category"));
        if (const int idx = typeCombo->findData(rule.scope); idx != -1)
            typeCombo->setCurrentIndex(idx);

        repoScopeLayout->addSpacing(10);
        repoScopeLayout->addWidget(new QLabel(tr("Scope") + ":", this));
        repoScopeLayout->addWidget(typeCombo, 1);
    }

    formLayout->addRow(tr("Repositories") + ":", repoScopeLayout);

    targetEdit = new QLineEdit(rule.rule, this);
    targetEdit->setPlaceholderText(RuleUtils::RULE_PATTERN_PLACEHOLDER);
    connect(targetEdit, &QLineEdit::returnPressed, this, &RuleEditDialog::onAccept);

    formLayout->addRow((mode == RuleMode::Exception ? tr("Exception Rule") : tr("Priority Rule")) + ":", targetEdit);
    mainLayout->addLayout(formLayout);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btnOk = buttonBox->button(QDialogButtonBox::Ok);
    btnCancel = buttonBox->button(QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RuleEditDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &RuleEditDialog::reject);

    buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(buttonBox);
    mainLayout->addLayout(buttonLayout);
}

/**
 * @brief Validates the target field and, if non-empty, stores the edited rule and accepts the dialog.
 */
void RuleEditDialog::onAccept() {
    const QString target = targetEdit->text().trimmed();
    if (target.isEmpty()) {
        QMessageBox::warning(this, tr("Empty Rule"), tr("The rule pattern cannot be empty."));
        targetEdit->setFocus();
        return;
    }

    result.type = mode == RuleMode::Exception ? QStringLiteral("exception") : QStringLiteral("priority");
    result.repo = repoCombo->currentData().toString();
    result.scope = mode == RuleMode::Exception ? typeCombo->currentData().toString() : QStringLiteral("package");
    result.rule = target;

    this->accept();
}
