/**
 * @file RulesManagerDialog.cpp
 * @brief Implementation of the RulesManagerDialog interface for managing system rule sets.
 */

#include <QHeaderView>

#include <RuleEditDialog.hpp>
#include <RulesManagerDialog.hpp>
#include <RuleUtils.hpp>

/**
 * @brief Sets up window modality, initial bounds, and instantiates the entire tabbed UI layout structures for rules.
 * @param parent Pointer to the parent widget container.
 */
RulesManagerDialog::RulesManagerDialog(QWidget *parent) : Dialog(parent, Qt::ApplicationModal) {
    this->setWindowTitle(tr("Priorities and Exceptions Rules Manager"));
    this->setMinimumSize(768, 432);
    this->fixed();

    tabWidget = new QTabWidget(this);
    exceptionsContainer = new QWidget(tabWidget);
    exceptionsLayout = new QVBoxLayout(exceptionsContainer);

    repoCombo = new QComboBox(exceptionsContainer);
    typeCombo = new QComboBox(exceptionsContainer);
    typeCombo->addItem(tr("By Package"), QStringLiteral("package"));
    typeCombo->addItem(tr("By Category"), QStringLiteral("category"));

    targetEdit = new QLineEdit(exceptionsContainer);
    targetEdit->setPlaceholderText(RuleUtils::RULE_PATTERN_PLACEHOLDER);
    connect(targetEdit, &QLineEdit::returnPressed, this, &RulesManagerDialog::onAddExceptionClicked);

    addExceptionBtn = new QPushButton(tr("Add Exception"), exceptionsContainer);
    connect(addExceptionBtn, &QPushButton::clicked, this, &RulesManagerDialog::onAddExceptionClicked);

    excFormLayout = new QHBoxLayout();
    excFormLayout->addWidget(new QLabel(tr("Repository") + ":"));
    excFormLayout->addWidget(repoCombo);
    excFormLayout->addWidget(new QLabel(tr("Scope") + ":"));
    excFormLayout->addWidget(typeCombo);
    excFormLayout->addWidget(targetEdit);
    excFormLayout->addWidget(addExceptionBtn);

    exceptionsModel = new QStandardItemModel(0, 3, this);
    exceptionsModel->setHorizontalHeaderLabels({tr("Repository"), tr("Scope"), tr("Exception Rules")});

    exceptionsTable = new TableView(exceptionsContainer);
    exceptionsTable->setModel(exceptionsModel);
    exceptionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(exceptionsTable, &QTableView::doubleClicked, this, &RulesManagerDialog::onExceptionRowDoubleClicked);

    deleteExceptionBtn = new QPushButton(tr("Remove Selected"), exceptionsContainer);
    connect(deleteExceptionBtn, &QPushButton::clicked, this, &RulesManagerDialog::onDeleteExceptionClicked);

    editExceptionBtn = new QPushButton(tr("Edit Selected"), exceptionsContainer);
    connect(editExceptionBtn, &QPushButton::clicked, this, &RulesManagerDialog::onEditExceptionClicked);

    excBottomLayout = new QHBoxLayout();
    excBottomLayout->addStretch();
    excBottomLayout->addWidget(editExceptionBtn);
    excBottomLayout->addWidget(deleteExceptionBtn);

    exceptionsLayout->addLayout(excFormLayout);
    exceptionsLayout->addWidget(exceptionsTable);
    exceptionsLayout->addLayout(excBottomLayout);

    prioritiesContainer = new QWidget(tabWidget);
    prioritiesLayout = new QVBoxLayout(prioritiesContainer);

    infoLabel = new QLabel(prioritiesContainer);
    infoLabel->setText(tr("Define preferred source repositories for specific packages when duplicates exist."));
    infoLabel->setWordWrap(true);

    QFont infoFont = infoLabel->font();
    infoFont.setItalic(true);
    infoLabel->setFont(infoFont);

    QPalette infoPalette = infoLabel->palette();
    infoPalette.setColor(QPalette::WindowText, infoPalette.color(QPalette::Active, QPalette::PlaceholderText));
    infoLabel->setPalette(infoPalette);

    priorityRepoCombo = new QComboBox(prioritiesContainer);
    priorityPackageEdit = new QLineEdit(prioritiesContainer);
    priorityPackageEdit->setPlaceholderText(RuleUtils::RULE_PATTERN_PLACEHOLDER);
    connect(priorityPackageEdit, &QLineEdit::returnPressed, this, &RulesManagerDialog::onAddPriorityClicked);

    addPriorityBtn = new QPushButton(tr("Add Priority"), prioritiesContainer);
    connect(addPriorityBtn, &QPushButton::clicked, this, &RulesManagerDialog::onAddPriorityClicked);

    priFormLayout = new QHBoxLayout();
    priFormLayout->addWidget(new QLabel(tr("Repository") + ":"));
    priFormLayout->addWidget(priorityRepoCombo);
    priFormLayout->addWidget(priorityPackageEdit);
    priFormLayout->addWidget(addPriorityBtn);

    prioritiesModel = new QStandardItemModel(0, 2, this);
    prioritiesModel->setHorizontalHeaderLabels({tr("Priority Rules"), tr("Repository")});

    prioritiesTable = new TableView(prioritiesContainer);
    prioritiesTable->setModel(prioritiesModel);
    prioritiesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(prioritiesTable, &QTableView::doubleClicked, this, &RulesManagerDialog::onPriorityRowDoubleClicked);

    deletePriorityBtn = new QPushButton(tr("Remove Selected"), prioritiesContainer);
    connect(deletePriorityBtn, &QPushButton::clicked, this, &RulesManagerDialog::onDeletePriorityClicked);

    editPriorityBtn = new QPushButton(tr("Edit Selected"), prioritiesContainer);
    connect(editPriorityBtn, &QPushButton::clicked, this, &RulesManagerDialog::onEditPriorityClicked);

    priBottomLayout = new QHBoxLayout();
    priBottomLayout->addStretch();
    priBottomLayout->addWidget(editPriorityBtn);
    priBottomLayout->addWidget(deletePriorityBtn);

    prioritiesLayout->addWidget(infoLabel);
    prioritiesLayout->addLayout(priFormLayout);
    prioritiesLayout->addWidget(prioritiesTable);
    prioritiesLayout->addLayout(priBottomLayout);

    tabWidget->addTab(exceptionsContainer, tr("Exceptions"));
    tabWidget->addTab(prioritiesContainer, tr("Priorities"));

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
}

/**
 * @brief Populates the repository filter combo box in the Exceptions tab.
 * @param repos List of repository names/identifiers.
 */
void RulesManagerDialog::setRepositories(const QStringList &repos) const {
    RuleUtils::populateRepoCombo(repoCombo, repos, true);
    const_cast<RulesManagerDialog *>(this)->m_repos = repos;
}

/**
 * @brief Populates the preferred repository combo box in the Priorities tab.
 * @param repos List of repository names/identifiers.
 */
void RulesManagerDialog::setPriorityRepositories(const QStringList &repos) const {
    RuleUtils::populateRepoCombo(priorityRepoCombo, repos, false);
}

/**
 * @brief Populates the exception and priority tables with a given set of rules.
 * @param rules The list of RuleEntry objects to display in the dialog UI.
 */
void RulesManagerDialog::loadRules(const QList<RuleEntry> &rules) const {
    exceptionsModel->removeRows(0, exceptionsModel->rowCount());
    prioritiesModel->removeRows(0, prioritiesModel->rowCount());

    for (const RuleEntry &rule: rules)
        appendRuleToTable(rule);
}

/**
 * @brief Slot triggered when the user clicks to submit a new exception rule.
 */
void RulesManagerDialog::onAddExceptionClicked() {
    const QString target = targetEdit->text().trimmed();
    if (target.isEmpty())
        return;

    targetEdit->clear();
    emit ruleAdded(RuleEntry{
        QStringLiteral("exception"),
        repoCombo->currentData().toString(),
        typeCombo->currentData().toString(),
        target
    });
}

/**
 * @brief Slot triggered to remove the currently selected exception row from the tracking cache.
 */
void RulesManagerDialog::onDeleteExceptionClicked() {
    const QModelIndexList selectedRows = exceptionsTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return;

    QList<RuleEntry> rules;
    rules.reserve(selectedRows.size());
    for (const QModelIndex &idx: selectedRows)
        rules.append(ruleFromExceptionRow(idx.row()));

    emit ruleRemoved(rules);
}

/**
 * @brief Slot triggered when adding a new package to the absolute global priority index.
 */
void RulesManagerDialog::onAddPriorityClicked() {
    const QString pkg = priorityPackageEdit->text().trimmed();
    if (pkg.isEmpty())
        return;

    priorityPackageEdit->clear();
    emit ruleAdded(RuleEntry{
        QStringLiteral("priority"),
        priorityRepoCombo->currentData().toString(),
        QStringLiteral("package"),
        pkg
    });
}

/**
 * @brief Slot triggered to remove a package from the global priority list.
 */
void RulesManagerDialog::onDeletePriorityClicked() {
    const QModelIndexList selectedRows = prioritiesTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return;

    QList<RuleEntry> rules;
    rules.reserve(selectedRows.size());
    for (const QModelIndex &idx: selectedRows)
        rules.append(ruleFromPriorityRow(idx.row()));

    emit ruleRemoved(rules);
}

/**
 * @brief Opens the edit dialog for a double-clicked exception row and, if confirmed, emits ruleEdited.
 * @param index The model index of the double-clicked cell.
 */
void RulesManagerDialog::onExceptionRowDoubleClicked(const QModelIndex &index) {
    if (!index.isValid())
        return;

    editExceptionRow(index.row());
}

/**
 * @brief Opens the edit dialog for a double-clicked priority row and, if confirmed, emits ruleEdited.
 * @param index The model index of the double-clicked cell.
 */
void RulesManagerDialog::onPriorityRowDoubleClicked(const QModelIndex &index) {
    if (!index.isValid())
        return;

    editPriorityRow(index.row());
}

/**
 * @brief Slot triggered by the "Edit Selected" button in the Exceptions tab.
 */
void RulesManagerDialog::onEditExceptionClicked() {
    const QModelIndexList selectedRows = exceptionsTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return;

    editExceptionRow(selectedRows.first().row());
}

/**
 * @brief Slot triggered by the "Edit Selected" button in the Priorities tab.
 */
void RulesManagerDialog::onEditPriorityClicked() {
    const QModelIndexList selectedRows = prioritiesTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return;

    editPriorityRow(selectedRows.first().row());
}

/**
 * @brief Opens the edit dialog for an exception row and, if confirmed, applies and emits the change.
 * @param row The row index in exceptionsModel to edit.
 */
void RulesManagerDialog::editExceptionRow(const int row) {
    const RuleEntry oldRule = ruleFromExceptionRow(row);
    if (RuleEditDialog dialog(RuleMode::Exception, oldRule, m_repos, this); dialog.exec() == Accepted) {
        const RuleEntry newRule = dialog.updatedRule();
        emit ruleEdited(oldRule, newRule);
    }
}

/**
 * @brief Opens the edit dialog for a priority row and, if confirmed, applies and emits the change.
 * @param row The row index in prioritiesModel to edit.
 */
void RulesManagerDialog::editPriorityRow(const int row) {
    const RuleEntry oldRule = ruleFromPriorityRow(row);
    if (RuleEditDialog dialog(RuleMode::Priority, oldRule, m_repos, this); dialog.exec() == Accepted) {
        const RuleEntry newRule = dialog.updatedRule();
        emit ruleEdited(oldRule, newRule);
    }
}

/**
 * @brief Reconstructs a RuleEntry from a row in the exceptions table.
 * @param row The row index in exceptionsModel.
 * @return The corresponding RuleEntry.
 */
RuleEntry RulesManagerDialog::ruleFromExceptionRow(const int row) const {
    return RuleEntry{
        QStringLiteral("exception"),
        exceptionsModel->item(row, 0)->data(Qt::UserRole).toString(),
        exceptionsModel->item(row, 1)->text(),
        exceptionsModel->item(row, 2)->text()
    };
}

/**
 * @brief Reconstructs a RuleEntry from a row in the priorities table.
 * @param row The row index in prioritiesModel.
 * @return The corresponding RuleEntry.
 */
RuleEntry RulesManagerDialog::ruleFromPriorityRow(const int row) const {
    return RuleEntry{
        QStringLiteral("priority"),
        prioritiesModel->item(row, 1)->data(Qt::UserRole).toString(),
        QStringLiteral("package"),
        prioritiesModel->item(row, 0)->text()
    };
}

/**
 * @brief Appends a single validated rule entry to its corresponding model view.
 * @param rule The RuleEntry instance to render in either the exception or priority model.
 */
void RulesManagerDialog::appendRuleToTable(const RuleEntry &rule) const {
    auto *repoItem = new QStandardItem(RuleUtils::displayRepositoryName(rule.repo)); // NOLINT
    repoItem->setData(rule.repo, Qt::UserRole);

    if (rule.type == QLatin1String("exception")) {
        const QList rowItems = {repoItem, new QStandardItem(rule.scope), new QStandardItem(rule.rule)};
        exceptionsModel->appendRow(rowItems);
    } else if (rule.type == QLatin1String("priority")) {
        const QList rowItems = {new QStandardItem(rule.rule), repoItem};
        prioritiesModel->appendRow(rowItems);
    }
}

/**
 * @brief Removes a rules entries matching the provided data from its model view.
 * @param rules The RuleEntry criteria used to locate and remove the matching row.
 */
void RulesManagerDialog::removeRulesFromTable(const QList<RuleEntry> &rules) const {
    for (const RuleEntry &rule: rules) {
        QStandardItemModel *model = (rule.type == QLatin1String("exception")) ? exceptionsModel : prioritiesModel;
        if (const int row = RuleUtils::findRuleRow(model, rule); row != -1)
            model->removeRow(row);
    }
}

/**
 * @brief Replaces the row matching @p oldRule with the values from @p newRule, in place.
 * @param oldRule The rule currently displayed, used to locate the row.
 * @param newRule The rule values to write into that row.
 */
void RulesManagerDialog::replaceRuleInTable(const RuleEntry &oldRule, const RuleEntry &newRule) const {
    const auto *model = oldRule.type == QLatin1String("exception") ? exceptionsModel : prioritiesModel;
    const int row = RuleUtils::findRuleRow(model, oldRule);
    if (row == -1)
        return;

    model->item(row, 0)->setText(RuleUtils::displayRepositoryName(newRule.repo));
    model->item(row, 0)->setData(newRule.repo, Qt::UserRole);

    if (oldRule.type == QLatin1String("exception")) {
        model->item(row, 1)->setText(newRule.scope);
        model->item(row, 2)->setText(newRule.rule);
    } else {
        model->item(row, 1)->setText(newRule.rule);
    }
}
