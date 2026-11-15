/**
 * @file NewConfigDialog.cpp
 * @brief Implementation of the dialog interface for resolving Slackware .new files.
 */

#include <QHeaderView>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>

#include <NewConfigDialog.hpp>
#include <SlackwareDefines.hpp>

/**
 * @brief Constructs the dialog, initializes widgets, and spawns the background worker thread.
 * @param items Initial list of detected .new files.
 * @param helperC Pointer to the privileged helper controller.
 * @param parent Optional parent widget.
 */
NewConfigDialog::NewConfigDialog(const QList<NewConfigFileItem> &items, HelperController *helperC, QWidget *parent)
    : Dialog(parent, Qt::ApplicationModal), helper(helperC), fileItems(items) {
    this->setWindowTitle(tr("Slackware .new Configuration Files"));
    this->setMinimumSize(960, 540);

    tableModel = new QStandardItemModel(this);
    tableModel->setHorizontalHeaderLabels({tr("Action"), tr("Target File")});

    fileTable = new TableView(this);
    fileTable->setModel(tableModel);
    fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(fileTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &NewConfigDialog::onSelectionChanged);

    btnKeepAll = new QPushButton(tr("Keep All"), this);
    btnOverwriteAll = new QPushButton(tr("Overwrite All"), this);
    btnRemoveAll = new QPushButton(tr("Remove All"), this);

    btnReadAsRoot = new QPushButton(tr("Read as Root"), this);
    btnReadAsRoot->setEnabled(false);
    btnReadAsRoot->setToolTip(tr("Read this configuration file with root privileges if permission was denied."));
    connect(btnReadAsRoot, &QPushButton::clicked, this, &NewConfigDialog::onReadAsRootClicked);

    connect(btnKeepAll, &QPushButton::clicked, this, [this] { setActionForAll(NewConfigAction::Keep); });
    connect(btnOverwriteAll, &QPushButton::clicked, this, [this] { setActionForAll(NewConfigAction::Overwrite); });
    connect(btnRemoveAll, &QPushButton::clicked, this, [this] { setActionForAll(NewConfigAction::Remove); });

    bulkLayout = new QHBoxLayout();
    bulkLayout->addWidget(btnKeepAll);
    bulkLayout->addWidget(btnOverwriteAll);
    bulkLayout->addWidget(btnRemoveAll);
    bulkLayout->addStretch();
    bulkLayout->addWidget(btnReadAsRoot);

    btnKeepSelected = new QPushButton(tr("Keep"), this);
    btnOverwriteSel = new QPushButton(tr("Overwrite"), this);
    btnRemoveSelected = new QPushButton(tr("Remove"), this);

    connect(btnKeepSelected, &QPushButton::clicked, this, [this] { setActForSelected(NewConfigAction::Keep); });
    connect(btnOverwriteSel, &QPushButton::clicked, this, [this] { setActForSelected(NewConfigAction::Overwrite); });
    connect(btnRemoveSelected, &QPushButton::clicked, this, [this] { setActForSelected(NewConfigAction::Remove); });

    itemActionsLayout = new QHBoxLayout();
    itemActionsLayout->addWidget(new QLabel(tr("Action on Selected File:"), this), 0, Qt::AlignTop);
    itemActionsLayout->addSpacing(5);
    itemActionsLayout->addWidget(btnKeepSelected);
    itemActionsLayout->addWidget(btnOverwriteSel);
    itemActionsLayout->addWidget(btnRemoveSelected);
    itemActionsLayout->addStretch();

    leftContainer = new QWidget(this);
    leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addLayout(bulkLayout);
    leftLayout->addWidget(fileTable);
    leftLayout->addSpacing(5);
    leftLayout->addLayout(itemActionsLayout);

    QFont boldFont = this->font();
    boldFont.setBold(true);

    lblDiffTitle = new QLabel(tr("Diff:"), this);
    lblDiffTitle->setFont(boldFont);

    lblOldPath = new QLabel(tr("Select a file to inspect differences"), this);
    lblOldPath->setFont(boldFont);

    lblNewPath = new QLabel(this);
    lblNewPath->setFont(boldFont);

    pathsLayout = new QVBoxLayout();
    pathsLayout->setSpacing(0);
    pathsLayout->addWidget(lblOldPath);
    pathsLayout->addWidget(lblNewPath);

    headerLayout = new QHBoxLayout();
    headerLayout->addSpacing(5);
    headerLayout->addWidget(lblDiffTitle, 0, Qt::AlignTop);
    headerLayout->addLayout(pathsLayout, 1);

    diffView = new QTextEdit(this);
    diffView->setReadOnly(true);
    diffView->setLineWrapMode(QTextEdit::NoWrap);
    diffView->setFontFamily(QStringLiteral("monospace"));

    btnApply = new QPushButton(tr("Apply Operations"), this);
    btnClose = new QPushButton(tr("Close"), this);
    connect(btnApply, &QPushButton::clicked, this, &NewConfigDialog::onApplyClicked);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);

    bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnApply);
    bottomLayout->addWidget(btnClose);

    rightContainer = new QWidget(this);
    rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addLayout(headerLayout);
    rightLayout->addWidget(diffView);
    rightLayout->addSpacing(5);
    rightLayout->addLayout(bottomLayout);

    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(12);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(leftContainer);
    splitter->addWidget(rightContainer);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitter);

    populateTable();

    diffWorker = new NewConfigWorker();
    diffWorker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::finished, diffWorker, &QObject::deleteLater);
    connect(this, &NewConfigDialog::requestDiffComputation, diffWorker, &NewConfigWorker::processDiff);
    connect(diffWorker, &NewConfigWorker::diffFinished, this, &NewConfigDialog::onDiffComputed);

    if (helper)
        connect(helper, &HelperController::rootConfigDataReceived, this, &NewConfigDialog::onRootConfigDataReceived);

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
 * @brief Populates the QTableView rows with file data and current action state.
 */
void NewConfigDialog::populateTable() {
    tableModel->setRowCount(0);

    for (const auto &item: fileItems) {
        QString actionStr;
        switch (item.action) {
            case NewConfigAction::Keep:
                actionStr = tr("Keep");
                break;
            case NewConfigAction::Overwrite:
                actionStr = tr("Overwrite");
                break;
            case NewConfigAction::Remove:
                actionStr = tr("Remove");
                break;
        }

        tableModel->appendRow({new QStandardItem(actionStr), new QStandardItem(item.originalFilePath)});
    }
}

/**
 * @brief Responds to table row selection changes by delegating diff work to the background thread or cache.
 */
void NewConfigDialog::onSelectionChanged() {
    const auto selected = fileTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        clearDiffHeader();
        diffView->clear();
        btnReadAsRoot->setEnabled(false);
        return;
    }

    const int row = selected.first().row();
    if (row < 0 || row >= fileItems.size())
        return;

    const auto &item = fileItems.at(row);
    currentSelectedOldPath = item.originalFilePath;
    currentSelectedNewPath = item.newFilePath;

    if (rootDiffCache.contains(item.newFilePath)) {
        btnReadAsRoot->setEnabled(false);
        setDiffHeader(item.originalFilePath, item.newFilePath, true);
        applyDiffToView(rootDiffCache.value(item.newFilePath));
        return;
    }

    btnReadAsRoot->setEnabled(false);

    ++currentRequestId;
    lblDiffTitle->setText(tr("Comparing:"));
    lblOldPath->setText(item.originalFilePath);
    lblNewPath->clear();
    diffView->clear();
    diffView->setPlainText(tr("Calculating diff in background..."));

    emit requestDiffComputation(currentRequestId, item.originalFilePath, item.newFilePath);
}

/**
 * @brief Receives the computed diff results and displays them or enables root button if permission denied.
 * @param requestId Monotonic ID used to discard obsolete diff results.
 * @param oldPath Original file path.
 * @param newPath New file path.
 * @param diffLines The list of calculated DiffLine items.
 * @param oldExists True if the baseline configuration file exists.
 * @param permissionDenied True if file read failed due to permission denial.
 */
void NewConfigDialog::onDiffComputed(const quint64 requestId, const QString &oldPath, const QString &newPath,
                                     const QList<DiffEngine::DiffLine> &diffLines, const bool oldExists,
                                     const bool permissionDenied) const {
    if (requestId != currentRequestId)
        return;

    if (permissionDenied) {
        btnReadAsRoot->setEnabled(true);
        lblDiffTitle->setText(tr("Denied:"));
        lblOldPath->setText(oldPath);
        lblNewPath->setText(newPath);
        diffView->setPlainText(tr("Permission denied reading configuration file.\n"
            "Click 'Read as Root' button above to inspect this file using root privileges."));
        return;
    }

    btnReadAsRoot->setEnabled(false);
    setDiffHeader(oldPath, newPath, false, oldExists);
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

    constexpr QColor addBg(46, 204, 113, 50), remBg(218, 68, 83, 50);
    const QColor textColor = palette().color(QPalette::Text);

    for (const auto &line: diffLines) {
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
 * @brief Requests the helper daemon to read the current files with root privileges.
 */
void NewConfigDialog::onReadAsRootClicked() {
    if (!helper || currentSelectedNewPath.isEmpty())
        return;

    diffView->setPlainText(tr("Requesting file contents with root privileges..."));

    auto sendRequest = [this] {
        const QString payload =
                QStringLiteral("READ_CONFIG_FILES:") + currentSelectedOldPath + u'|' + currentSelectedNewPath + SEP;
        helper->socket()->write(payload.toUtf8());
    };

    if (!helper->socket() || helper->socket()->state() != QLocalSocket::ConnectedState) {
        helper->setupHelper(PendingAction::None);
        connect(helper, &HelperController::helperReady, this, sendRequest, Qt::SingleShotConnection);
    } else {
        sendRequest();
    }
}

/**
 * @brief Slot called when root-privileged file data is received from the helper daemon.
 * @param oldText Content of the baseline file read by root.
 * @param newText Content of the .new file read by root.
 * @param oldPath Baseline file path.
 * @param newPath Incoming .new file path.
 */
void NewConfigDialog::onRootConfigDataReceived(const QString &oldText, const QString &newText,
                                               const QString &oldPath, const QString &newPath) {
    const QList<DiffEngine::DiffLine> diff = DiffEngine::computeDiff(oldText, newText);

    rootDiffCache.insert(newPath, diff);

    if (newPath == currentSelectedNewPath) {
        btnReadAsRoot->setEnabled(false);
        setDiffHeader(oldPath, newPath, true);
        applyDiffToView(diff);
    }
}

/**
 * @brief Sets the resolution action for all selected files in the table.
 * @param action Desired NewConfigAction.
 */
void NewConfigDialog::setActForSelected(const NewConfigAction action) {
    const auto selected = fileTable->selectionModel()->selectedRows();
    QString actionStr;
    switch (action) {
        case NewConfigAction::Keep:
            actionStr = tr("Keep");
            break;
        case NewConfigAction::Overwrite:
            actionStr = tr("Overwrite");
            break;
        case NewConfigAction::Remove:
            actionStr = tr("Remove");
            break;
    }

    for (const auto &idx: selected) {
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
        case NewConfigAction::Keep:
            actionStr = tr("Keep");
            break;
        case NewConfigAction::Overwrite:
            actionStr = tr("Overwrite");
            break;
        case NewConfigAction::Remove:
            actionStr = tr("Remove");
            break;
    }

    for (int i = 0; i < fileItems.size(); ++i) {
        fileItems[i].action = action;
        tableModel->item(i, 0)->setText(actionStr);
    }
}

/**
 * @brief Updates the dialog's file list when files have been modified or rescanned.
 * @param newItems Updated list of .new configuration items.
 */
void NewConfigDialog::updateFileList(const QList<NewConfigFileItem> &newItems) {
    fileItems = newItems;
    populateTable();

    QSet<QString> activeNewPaths;
    for (const auto &item: fileItems)
        activeNewPaths.insert(item.newFilePath);

    for (auto it = rootDiffCache.begin(); it != rootDiffCache.end();) {
        if (!activeNewPaths.contains(it.key()))
            it = rootDiffCache.erase(it);
        else
            ++it;
    }

    if (!fileItems.isEmpty()) {
        fileTable->selectRow(0);
        onSelectionChanged();
    } else {
        lblDiffTitle->setText(tr("Status:"));
        lblOldPath->setText(tr("All configuration files resolved."));
        lblNewPath->clear();
        diffView->clear();
        btnReadAsRoot->setEnabled(false);
    }
}

/**
 * @brief Updates the diff header paths aligned vertically in a grid layout.
 * @param oldPath Original file path.
 * @param newP Incoming .new file path.
 * @param isRoot True if loaded with root privileges.
 * @param oldExists True if the baseline file exists on disk.
 */
void NewConfigDialog::setDiffHeader(const QString &oldPath, const QString &newP, const bool isRoot,
                                    const bool oldExists) const {
    lblDiffTitle->setText(isRoot ? tr("Diff (Root):") : tr("Diff:"));

    if (oldExists) {
        lblOldPath->setText(oldPath);
    } else {
        lblOldPath->setText(tr("(Original does not exist — brand new file)"));
    }

    lblNewPath->setText(newP);
}

/**
 * @brief Resets the diff header labels to the default state.
 */
void NewConfigDialog::clearDiffHeader() const {
    lblDiffTitle->setText(tr("Diff:"));
    lblOldPath->setText(tr("No file selected"));
    lblNewPath->clear();
}
