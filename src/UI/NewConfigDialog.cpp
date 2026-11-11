/**
 * @file NewConfigDialog.cpp
 * @brief Implementation of the dialog interface for resolving Slackware .new files.
 */

#include <QApplication>
#include <QHeaderView>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>

#include <NewConfigDialog.hpp>

/**
 * @brief Constructs the dialog, initializes widgets, and spawns the background worker thread.
 * @param items Initial list of detected .new files.
 * @param parent Optional parent widget.
 */
NewConfigDialog::NewConfigDialog(const QList<NewConfigFileItem> &items, QWidget *parent)
    : Dialog(parent, Qt::ApplicationModal), fileItems(items) {
    this->setWindowTitle(tr("Slackware .new Configuration Files"));
    // this->setMinimumSize(1024, 576);

    setupUi();
    populateTable();

    diffWorker = new NewConfigWorker();
    diffWorker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::finished, diffWorker, &QObject::deleteLater);
    connect(this, &NewConfigDialog::requestDiffComputation, diffWorker, &NewConfigWorker::processDiff);
    connect(diffWorker, &NewConfigWorker::diffFinished, this, &NewConfigDialog::onDiffComputed);

    workerThread.start();

    if (!fileItems.isEmpty()) {
        fileTable->selectRow(0);
        onSelectionChanged();
    }
}

/**
 * @brief Destructor that terminates the background thread and cleans up models.
 */
NewConfigDialog::~NewConfigDialog() {
    workerThread.quit();
    workerThread.wait();

    if (tableModel)
        tableModel->clear();
}

/**
 * @brief Assembles layout containers, buttons, file table, and diff viewer.
 */
void NewConfigDialog::setupUi() {
    tableModel = new QStandardItemModel(this);
    tableModel->setHorizontalHeaderLabels({tr("Action"), tr("Target File")});

    fileTable = new TableView(this);
    fileTable->setModel(tableModel);
    fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    fileTable->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(fileTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &NewConfigDialog::onSelectionChanged);

    auto *bulkLayout = new QHBoxLayout();
    btnKeepAll = new QPushButton(tr("Keep All (K)"), this);
    btnOverwriteAll = new QPushButton(tr("Overwrite All (O)"), this);
    btnRemoveAll = new QPushButton(tr("Remove All (R)"), this);

    connect(btnKeepAll, &QPushButton::clicked, this, [this] { setActionForAll(NewConfigAction::Keep); });
    connect(btnOverwriteAll, &QPushButton::clicked, this, [this] { setActionForAll(NewConfigAction::Overwrite); });
    connect(btnRemoveAll, &QPushButton::clicked, this, [this] { setActionForAll(NewConfigAction::Remove); });

    bulkLayout->addWidget(btnKeepAll);
    bulkLayout->addWidget(btnOverwriteAll);
    bulkLayout->addWidget(btnRemoveAll);

    auto *itemActionsLayout = new QHBoxLayout();
    btnKeepSelected = new QPushButton(tr("Keep Selected"), this);
    btnOverwriteSelected = new QPushButton(tr("Overwrite Selected"), this);
    btnRemoveSelected = new QPushButton(tr("Remove Selected"), this);

    connect(btnKeepSelected, &QPushButton::clicked, this, [this] { setActionForSelected(NewConfigAction::Keep); });
    connect(btnOverwriteSelected, &QPushButton::clicked, this, [this] { setActionForSelected(NewConfigAction::Overwrite); });
    connect(btnRemoveSelected, &QPushButton::clicked, this, [this] { setActionForSelected(NewConfigAction::Remove); });

    itemActionsLayout->addWidget(btnKeepSelected);
    itemActionsLayout->addWidget(btnOverwriteSelected);
    itemActionsLayout->addWidget(btnRemoveSelected);

    auto *leftContainer = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel(tr("Bulk Actions:"), this));
    leftLayout->addLayout(bulkLayout);
    leftLayout->addWidget(fileTable);
    leftLayout->addWidget(new QLabel(tr("Action on Selected Files:"), this));
    leftLayout->addLayout(itemActionsLayout);

    QFont font = this->font();
    font.setBold(true);

    diffHeaderLabel = new QLabel(tr("Select a file to inspect differences"), this);
    diffHeaderLabel->setFont(font);

    diffView = new QTextEdit(this);
    diffView->setReadOnly(true);
    diffView->setLineWrapMode(QTextEdit::NoWrap);
    diffView->setFontFamily(QStringLiteral("monospace"));

    auto *rightContainer = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(diffHeaderLabel);
    rightLayout->addWidget(diffView);

    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftContainer);
    splitter->addWidget(rightContainer);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    btnApply = new QPushButton(tr("Apply Decisions"), this);
    btnClose = new QPushButton(tr("Close"), this);
    connect(btnApply, &QPushButton::clicked, this, &NewConfigDialog::onApplyClicked);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnApply);
    bottomLayout->addWidget(btnClose);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitter);
    mainLayout->addLayout(bottomLayout);
}

/**
 * @brief Populates the QTableView rows with file data and current action state.
 */
void NewConfigDialog::populateTable() {
    tableModel->setRowCount(0);

    for (const auto &item : fileItems) {
        QString actionStr;
        switch (item.action) {
            case NewConfigAction::Keep: actionStr = tr("Keep"); break;
            case NewConfigAction::Overwrite: actionStr = tr("Overwrite"); break;
            case NewConfigAction::Remove: actionStr = tr("Remove"); break;
        }

        auto *actionItem = new QStandardItem(actionStr);
        auto *pathItem = new QStandardItem(item.originalFilePath);

        tableModel->appendRow({actionItem, pathItem});
    }
}

/**
 * @brief Responds to table row selection changes by delegating diff work to the background thread.
 */
void NewConfigDialog::onSelectionChanged() {
    const auto selected = fileTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        diffHeaderLabel->setText(tr("No file selected"));
        diffView->clear();
        return;
    }

    const int row = selected.first().row();
    if (row < 0 || row >= fileItems.size())
        return;

    const auto &item = fileItems.at(row);

    ++currentRequestId;

    diffHeaderLabel->setText(tr("Comparing: %1 (loading...)").arg(item.originalFilePath));
    diffView->clear();
    diffView->setPlainText(tr("Calculating diff in background..."));

    emit requestDiffComputation(currentRequestId, item.originalFilePath, item.newFilePath);
}

/**
 * @brief Receives the computed diff results and displays them if still relevant.
 * @param requestId Monotonic ID used to discard obsolete diff results.
 * @param oldPath Original file path.
 * @param newPath New file path.
 * @param diffLines The list of calculated DiffLine items.
 * @param oldExists True if the baseline configuration file exists.
 */
void NewConfigDialog::onDiffComputed(const quint64 requestId, const QString &oldPath, const QString &newPath,
                                     const QList<DiffEngine::DiffLine> &diffLines, const bool oldExists) const {
    if (requestId != currentRequestId)
        return;

    diffHeaderLabel->setText(oldExists
        ? tr("Diff: %1 vs %2").arg(oldPath, newPath)
        : tr("Diff: %1 (Original does not exist — brand new file)").arg(newPath));

    applyDiffToView(diffLines);
}

/**
 * @brief Inserts the colored diff text into the QTextEdit using native blocks, avoiding redraw lag.
 * @param diffLines Sequence of DiffLine objects to render.
 */
void NewConfigDialog::applyDiffToView(const QList<DiffEngine::DiffLine> &diffLines) const {
    diffView->setUpdatesEnabled(false);
    diffView->clear();

    QTextCursor cursor(diffView->document());
    cursor.beginEditBlock();

    QTextBlockFormat blockFormat;
    QTextCharFormat charFormat;

    const QColor addBg(46, 204, 113, 50);
    const QColor remBg(218, 68, 83, 50);
    const QColor textColor = palette().color(QPalette::Text);

    for (const auto &line : diffLines) {
        if (line.type == DiffEngine::LineType::Added) {
            blockFormat.setBackground(addBg);
            charFormat.setForeground(textColor);
            cursor.setBlockFormat(blockFormat);
            cursor.setCharFormat(charFormat);
            cursor.insertText(QStringLiteral("+ %1\n").arg(line.text));
        } else if (line.type == DiffEngine::LineType::Removed) {
            blockFormat.setBackground(remBg);
            charFormat.setForeground(textColor);
            cursor.setBlockFormat(blockFormat);
            cursor.setCharFormat(charFormat);
            cursor.insertText(QStringLiteral("- %1\n").arg(line.text));
        } else {
            blockFormat.setBackground(Qt::transparent);
            charFormat.setForeground(textColor);
            cursor.setBlockFormat(blockFormat);
            cursor.setCharFormat(charFormat);
            cursor.insertText(QStringLiteral("  %1\n").arg(line.text));
        }
    }

    cursor.endEditBlock();
    diffView->setUpdatesEnabled(true);
}

/**
 * @brief Sets the resolution action for all selected files in the table.
 * @param action Desired NewConfigAction.
 */
void NewConfigDialog::setActionForSelected(const NewConfigAction action) {
    const auto selected = fileTable->selectionModel()->selectedRows();
    QString actionStr;
    switch (action) {
        case NewConfigAction::Keep: actionStr = tr("Keep"); break;
        case NewConfigAction::Overwrite: actionStr = tr("Overwrite"); break;
        case NewConfigAction::Remove: actionStr = tr("Remove"); break;
    }

    for (const auto &idx : selected) {
        const int row = idx.row();
        fileItems[row].action = action;
        tableModel->item(row, 0)->setText(actionStr);
    }
}

/**
 * @brief Sets the resolution action for all files present in the list.
 * @param action Desired NewConfigAction.
 */
void NewConfigDialog::setActionForAll(const NewConfigAction action) {
    QString actionStr;
    switch (action) {
        case NewConfigAction::Keep: actionStr = tr("Keep"); break;
        case NewConfigAction::Overwrite: actionStr = tr("Overwrite"); break;
        case NewConfigAction::Remove: actionStr = tr("Remove"); break;
    }

    for (int i = 0; i < fileItems.size(); ++i) {
        fileItems[i].action = action;
        tableModel->item(i, 0)->setText(actionStr);
    }
}

/**
 * @brief Emits the apply signal without closing the dialog.
 */
void NewConfigDialog::onApplyClicked() {
    emit applyResolutionRequested(resolvedItems());
}

/**
 * @brief Returns the list of items with their configured resolution actions.
 * @return Resolved configuration file list.
 */
QList<NewConfigFileItem> NewConfigDialog::resolvedItems() const {
    return fileItems;
}

/**
 * @brief Updates the dialog's file list when files have been modified or rescanned.
 * @param newItems Updated list of .new configuration items.
 */
void NewConfigDialog::updateFileList(const QList<NewConfigFileItem> &newItems) {
    fileItems = newItems;
    populateTable();

    if (!fileItems.isEmpty()) {
        fileTable->selectRow(0);
        onSelectionChanged();
    } else {
        diffHeaderLabel->setText(tr("All configuration files resolved."));
        diffView->clear();
    }
}
