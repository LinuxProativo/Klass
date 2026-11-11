/**
 * @file NewConfigDialog.cpp
 * @brief Implementation of NewConfigDialog with non-HTML syntax-styled diff view.
 */

#include <QApplication>
#include <QFile>
#include <DiffEngine.hpp>
#include <NewConfigDialog.hpp>
#include <SlackwareDefines.hpp>

NewConfigDialog::NewConfigDialog(const QList<NewConfigFileItem> &items, QWidget *parent)
    : Dialog(parent, Qt::ApplicationModal), fileItems(items) {
    this->setWindowTitle(tr("Slackware .new Configuration Files"));
    this->setMinimumSize(960, 540);

    setupUi();
    populateTable();

    if (!fileItems.isEmpty()) {
        fileTable->selectRow(0);
        onSelectionChanged();
    }
}

NewConfigDialog::~NewConfigDialog() {
    if (tableModel)
        tableModel->clear();
}

void NewConfigDialog::setupUi() {
    // Left: Table & File actions
    tableModel = new QStandardItemModel(this);
    tableModel->setHorizontalHeaderLabels({tr("Action"), tr("Target File"), tr("Status")});

    fileTable = new TableView(this);
    fileTable->setModel(tableModel);
    fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    fileTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(fileTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &NewConfigDialog::onSelectionChanged);

    // Bulk buttons
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

    // Selected item buttons
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
    leftLayout->addWidget(new QLabel(tr("Global Actions:"), this));
    leftLayout->addLayout(bulkLayout);
    leftLayout->addWidget(fileTable);
    leftLayout->addWidget(new QLabel(tr("Action on Selection:"), this));
    leftLayout->addLayout(itemActionsLayout);

    // Right: Diff Viewer (Zero HTML)
    diffHeaderLabel = new QLabel(tr("Select a file to inspect differences"), this);
    diffHeaderLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));

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

    // Bottom action bar
    btnApply = new QPushButton(tr("Apply Decisions"), this);
    btnClose = new QPushButton(tr("Close"), this);
    connect(btnApply, &QPushButton::clicked, this, [this] {
        emit applyResolutionRequested(resolvedItems());
        this->accept();
    });
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnApply);
    bottomLayout->addWidget(btnClose);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitter);
    mainLayout->addLayout(bottomLayout);
}

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
        auto *statusItem = new QStandardItem(item.originalExists ? tr("Replaces existing") : tr("New file only"));

        tableModel->appendRow({actionItem, pathItem, statusItem});
    }
}

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
    diffHeaderLabel->setText(tr("Diff: %1 vs %2").arg(item.originalFilePath, item.newFilePath));
    renderDiff(item.originalFilePath, item.newFilePath);
}

void NewConfigDialog::renderDiff(const QString &oldPath, const QString &newPath) {
    diffView->clear();

    const QString oldContent = NewConfigManager::readFileContent(oldPath);
    const QString newContent = NewConfigManager::readFileContent(newPath);

    if (oldContent.isEmpty() && !QFile::exists(oldPath)) {
        QTextCursor cursor(diffView->document());
        cursor.insertText(tr("Existing original file does not exist.\nThis is a brand new configuration file.\n\n"));
        cursor.insertText(newContent);
        return;
    }

    const QList<DiffEngine::DiffLine> diffs = DiffEngine::computeDiff(oldContent, newContent);

    QTextCursor cursor(diffView->document());
    QTextBlockFormat blockFormat;
    QTextCharFormat charFormat;

    for (const auto &line : diffs) {
        if (line.type == DiffEngine::LineType::Added) {
            blockFormat.setBackground(QColor(46, 204, 113, 60)); // Green tint
            charFormat.setForeground(palette().color(QPalette::Text));
            cursor.setBlockFormat(blockFormat);
            cursor.setCharFormat(charFormat);
            cursor.insertText(QStringLiteral("+ %1\n").arg(line.text));
        } else if (line.type == DiffEngine::LineType::Removed) {
            blockFormat.setBackground(QColor(218, 68, 83, 60)); // Red tint
            charFormat.setForeground(palette().color(QPalette::Text));
            cursor.setBlockFormat(blockFormat);
            cursor.setCharFormat(charFormat);
            cursor.insertText(QStringLiteral("- %1\n").arg(line.text));
        } else {
            blockFormat.setBackground(Qt::transparent);
            charFormat.setForeground(palette().color(QPalette::Text));
            cursor.setBlockFormat(blockFormat);
            cursor.setCharFormat(charFormat);
            cursor.insertText(QStringLiteral("  %1\n").arg(line.text));
        }
    }
}

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

QList<NewConfigFileItem> NewConfigDialog::resolvedItems() const {
    return fileItems;
}