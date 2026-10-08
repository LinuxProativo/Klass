/**
 * @file RepositoryTab.cpp
 * @brief Interface providing a split-pane layout for categories, package lists, and others tabs.
 */

#include <QFile>
#include <QHeaderView>
#include <QMenu>

#include <CenterDelegate.hpp>
#include <ContextMenu.hpp>
#include <Debug.hpp>
#include <Icon.hpp>
#include <RepositoryTab.hpp>
#include <RepositoryTabUtils.hpp>

/**
 * @brief Constructs a new Repository Tab.
 * @param parent The parent widget.
 * @param isvisible Controls whether the category navigation tree is displayed.
 * @param packagesManager Pointer to the single instance managing system and repository packages.
 */
RepositoryTab::RepositoryTab(QWidget *parent, const bool isvisible, Packages *packagesManager) : QWidget(parent),
    packagesManager(packagesManager), showCategories(isvisible) {
    this->setAttribute(Qt::WA_StaticContents);

    catList = new TreeWidget();
    catList->setHeaderLabel(tr("Categories"));
    catList->setMaximumWidth(300);
    catList->setVisible(isvisible);
    connect(catList, &TreeWidget::itemClicked, this, &RepositoryTab::onCategoryItemClicked);

    packageModel = new PackageTableModel(this);
    proxyModel = new CategoryFilterProxyModel(this);
    proxyModel->setSourceModel(packageModel);

    packageTable = new TableView();
    packageTable->setShowHorizontalGrid(true);
    packageTable->setModel(proxyModel);
    packageTable->setItemDelegateForColumn(0, new CenterDelegate());
    packageTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    connect(packageTable, &QTableView::customContextMenuRequested, this, &RepositoryTab::showContextMenu);
    connect(packageTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &RepositoryTab::onSelectionChanged);

    versionDelegate = new VersionComboBoxDelegate(packageTable);
    connect(versionDelegate, &VersionComboBoxDelegate::versionChanged, this, &RepositoryTab::onVersionChanged);
    packageTable->setItemDelegateForColumn(2, versionDelegate);

    const Icon::IconManager icon;
    installAction = new QAction(icon.getIcon(Icon::Id::MarkInstall, COLOR_GREEN), tr("Install"), this);
    installAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    installAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    packageTable->addAction(installAction);
    connect(installAction, &QAction::triggered, this, &RepositoryTab::onInstallShortcut);

    upgradeAction = new QAction(icon.getIcon(Icon::Id::MarkUpdate, COLOR_PURPLE), tr("Upgrade"), this);
    upgradeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
    upgradeAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    packageTable->addAction(upgradeAction);
    connect(upgradeAction, &QAction::triggered, this, &RepositoryTab::onUpgradeShortcut);

    reinstallAction = new QAction(icon.getIcon(Icon::Id::MarkReinstall, COLOR_BLUE), tr("Reinstall"), this);
    reinstallAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    reinstallAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    packageTable->addAction(reinstallAction);
    connect(reinstallAction, &QAction::triggered, this, &RepositoryTab::onReinstallShortcut);

    rollbackAction = new QAction(icon.getIcon(Icon::Id::MarkRollback, COLOR_ORANGE), tr("Rollback"), this);
    rollbackAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    rollbackAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    packageTable->addAction(rollbackAction);

    removeAction = new QAction(icon.getIcon(Icon::Id::MarkRemove, COLOR_RED), tr("Remove"), this);
    removeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    removeAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    packageTable->addAction(removeAction);
    connect(removeAction, &QAction::triggered, this, &RepositoryTab::onRemoveShortcut);

    lockAction = new QAction(icon.getIcon(Icon::Id::MarkBlock, COLOR_YELLOW), tr("Lock"), this);
    lockAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    lockAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    packageTable->addAction(lockAction);

    priorityAct = new QAction(icon.getIcon(Icon::Id::MarkPriority, COLOR_PURPLE), tr("Prioritize"), this);
    priorityAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    priorityAct->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    packageTable->addAction(priorityAct);

    unselectAct = new QAction(icon.getIcon(Icon::Id::Unselect, QPalette().color(QPalette::Dark)), tr("Unselect"), this);
    unselectAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    unselectAct->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    packageTable->addAction(unselectAct);
    connect(unselectAct, &QAction::triggered, this, &RepositoryTab::onUnselectShortcut);

    infoText = new QTextEdit();
    infoText->setReadOnly(true);
    infoText->setLineWrapMode(QTextEdit::NoWrap);

    fileList = new TreeWidget();
    fileList->setHeaderLabel(tr("File List"));

    detailsTab = new QTabWidget();
    detailsTab->addTab(infoText, tr("Info"));
    detailsTab->addTab(fileList, tr("Files"));
    detailsTab->setMinimumWidth(250);

    splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(catList);
    splitter->addWidget(packageTable);
    splitter->addWidget(detailsTab);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 1);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);
    splitter->setCollapsible(2, false);

    layout = new QVBoxLayout(this);
    layout->addWidget(splitter);

    fieldFormat.setLineHeight(150, QTextBlockFormat::ProportionalHeight);
    depMidFormat.setLineHeight(100, QTextBlockFormat::ProportionalHeight);
    depLastFormat.setLineHeight(150, QTextBlockFormat::ProportionalHeight);
    descFormat.setLineHeight(100, QTextBlockFormat::ProportionalHeight);
    boldFormat.setFontWeight(QFont::Bold);
    normalFormat.setFontWeight(QFont::Normal);
}

/**
 * @brief Filters the package table rows based on the selected category item.
 * @param item The tree widget item that was clicked, containing the category text.
 */
void RepositoryTab::onCategoryItemClicked(const QTreeWidgetItem *item) const {
    if (!item) return;
    if (item == root || item->parent() == nullptr) {
        proxyModel->setCategoryFilter(QString{});
    } else {
        proxyModel->setCategoryFilter(item->text(0));
    }
}

/**
 * @brief Handles version selection changes from the VersionComboBoxDelegate.
 * @param pIndex Model index of the cell being edited.
 * @param vIndex Selected index in the version dropdown.
 */
void RepositoryTab::onVersionChanged(const QModelIndex &pIndex, const int vIndex) {
    if (!pIndex.isValid())
        return;

    const QModelIndex index = proxyModel->mapToSource(pIndex);
    const int row = index.row();
    if (row < 0 || row >= packageModel->rowCount())
        return;

    auto &rowData = packageModel->rowAt(row);

    if (const auto &pkgList = rowData.pkgList; vIndex >= 0 && vIndex < pkgList.size()) {
        const PkgInfo &nPkg = pkgList[vIndex];
        const PkgInfo *metaPkgPtr{};
        const PkgInfo *slackwarePkgPtr{};
        bool rowHasPriorityWinner = false;

        for (const auto &pkg: pkgList) {
            if (!pkg.isInstalled && !metaPkgPtr)
                metaPkgPtr = &pkg;
            if (!slackwarePkgPtr && pkg.repoName.compare(SLACK_OFICIAL) == 0)
                slackwarePkgPtr = &pkg;
            if (ruleStatuses.value({pkg.name, pkg.version, pkg.repoName}, RuleSt::Normal) == RuleSt::Prioritized)
                rowHasPriorityWinner = true;
        }

        const PkgInfo &metaPkg = metaPkgPtr ? *metaPkgPtr : nPkg;
        const QString categorySource = slackwarePkgPtr ? slackwarePkgPtr->category : metaPkg.category;
        const RuleSt status = ruleStatuses.value({nPkg.name, nPkg.version, nPkg.repoName}, RuleSt::Normal);

        PkgStatus packageState;
        if (status == RuleSt::Excluded) {
            packageState = PkgStatus::Excluded;
        } else if (!rowData.actionSymbol.isEmpty() && nPkg.version == rowData.targetVersion) {
            if (rowData.actionSymbol == QStringLiteral("update"))
                packageState = PkgStatus::PendingUpdate;
            else
                packageState = nPkg.isInstalled ? PkgStatus::PendingReinstall : PkgStatus::PendingInstall;
        } else if (nPkg.isInstalled) {
            packageState = PkgStatus::Installed;
        } else {
            packageState = PkgStatus::Available;
        }

        rowData.status = packageState;
        rowData.isPrioritized = rowHasPriorityWinner;
        rowData.version = nPkg.version;
        rowData.description = metaPkg.description;
        rowData.category = categorySource;
        rowData.isInstalled = nPkg.isInstalled;

        QString targetRepoName = nPkg.repoName;
        if (nPkg.isInstalled) {
            for (const auto &pkg: pkgList) {
                if (!pkg.isInstalled && pkg.version == nPkg.version) {
                    targetRepoName = pkg.repoName;
                    break;
                }
            }
        }
        rowData.repoName = targetRepoName;

        packageModel->notifyRowChanged(row);
        Debug::msg("Package version changed to", "RepositoryTab", {DColor::Cyan, nPkg.version});
    }

    const QModelIndex proxyTarget0 = proxyModel->mapFromSource(packageModel->index(row, 0));
    const QModelIndex proxyTarget3 = proxyModel->mapFromSource(packageModel->index(row, 3));

    if (proxyTarget0.isValid()) {
        packageTable->selectionModel()->select(
            proxyTarget0,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
        );

        QItemSelection sel;
        sel.select(proxyTarget0, proxyTarget3);
        onSelectionChanged(sel, QItemSelection());
    }
}

/**
 * @brief Appends pending package files to the tree view in asynchronous chunks.
 */
void RepositoryTab::appendFileBatch() {
    const QString localSignature = currentLoadSignature;
    constexpr int BatchSize = 500;
    Debug::msg("Processing file batch for signature", "RepositoryTab", {DColor::Blue, localSignature});

    QList<QTreeWidgetItem *> items;
    items.reserve(BatchSize);

    const QFontMetrics metric(fileList->font());
    const int startIdx = pendingFileIndex;
    const int end = static_cast<int>(qMin(startIdx + BatchSize, pendingFiles.size()));

    pendingFileIndex = end;

    for (int i = startIdx; i < end; ++i) {
        if (localSignature != currentLoadSignature) {
            qDeleteAll(items);
            return;
        }

        const QString &file = pendingFiles.at(i);
        items.append(new QTreeWidgetItem(QStringList{file}));

        if (const int size = metric.horizontalAdvance(file); size > pendingMaxWidth)
            pendingMaxWidth = size;
    }

    if (localSignature != currentLoadSignature) {
        qDeleteAll(items);
        return;
    }

    fileList->addTopLevelItems(items);

    if (pendingFileIndex < pendingFiles.size()) {
        QTimer::singleShot(0, this, [this, localSignature] {
            if (localSignature == currentLoadSignature)
                appendFileBatch();
        });
        return;
    }

    if (pendingMaxWidth + 10 > fileList->viewport()->width()) {
        fileList->header()->setStretchLastSection(false);
        fileList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        fileList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    Debug::msg("Batch processing completed for signature", "RepositoryTab", {DColor::LightGreen, localSignature});
}

/**
 * @brief Displays detailed package information when a row is selected in the table view.
 * @param selected Newly selected ranges.
 * @param deselected Unselected ranges.
 */
void RepositoryTab::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    (void) deselected;
    if (selected.indexes().isEmpty()) return;

    const QModelIndex proxyIndex = selected.indexes().first();
    const QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    const int row = sourceIndex.row();
    if (row < 0 || row >= packageModel->rowCount()) return;

    const auto &rowData = packageModel->rowAt(row);

    QString repoName = rowData.repoName;
    if (repoName.isEmpty())
        repoName = this->property("repoName").toString();
    Debug::msg("Active repository context", "RepositoryTab", {DColor::Violet, repoName});

    if (repoName == SLACK_OTHERS)
        repoName.clear();

    const QString name = rowData.name;
    const QString version = rowData.version;
    const bool isInstalled = rowData.isInstalled;
    auto info = packagesManager->getPackageInfo(repoName, name, version);
    Debug::msg("Selected package target", "RepositoryTab", {DColor::Cyan, QString("%1-%2").arg(name, version)});

    if (info.name.isEmpty() && isInstalled && !repoName.isEmpty())
        info = packagesManager->getPackageInfo({}, name, version);

    const QString detailedDesc = info.detailedDesc;
    const QString comp = info.compressedSize;
    const QString uncomp = info.uncompressedSize;
    const QString required = info.required;
    const QString conflicts = info.conflicts;
    const QString suggests = info.suggests;
    const QString mirrorUrl = mirrorMap.value(repoName, QString{});
    const ChecksumEntry pkgEntry = packagesManager->getChecksumEntry(repoName, name, version, FileFormat::Package);
    const ChecksumEntry ascEntry = packagesManager->getChecksumEntry(repoName, name, version, FileFormat::Asc);

    infoText->clear();
    const QFontMetrics metrics(infoText->font());
    int maxLabelWidth = 0;

    const QStringList labels = {
        tr("Package Name") + ":",
        tr("Package Version") + ":",
        tr("Package Repository") + ":",
        tr("Mirror URL") + ":",
        tr("Package Size") + ":",
        tr("Installed Package Size") + ":",
        tr("Package MD5") + ":",
        tr("Signature MD5") + ":",
        tr("Repository Path") + ":",
        tr("Package Required") + ":",
        tr("Package Conflicts") + ":",
        tr("Package Suggests") + ":"
    };

    for (const QString &label: labels) {
        if (const int width = metrics.horizontalAdvance(label); width > maxLabelWidth)
            maxLabelWidth = width;
    }

    const int tabPosition = maxLabelWidth + 20;
    QList<QTextOption::Tab> tabs;
    tabs.append(QTextOption::Tab(tabPosition, QTextOption::LeftTab));

    fieldFormat.setTabPositions(tabs);
    depMidFormat.setLeftMargin(tabPosition);
    depLastFormat.setLeftMargin(tabPosition);

    QTextCursor cursor(infoText->textCursor());
    cursor.setBlockFormat(fieldFormat);

    appendField(cursor, labels.at(0), name);
    appendField(cursor, labels.at(1), version);
    appendField(cursor, labels.at(2), repoName);
    appendField(cursor, labels.at(3), mirrorUrl);
    appendField(cursor, labels.at(4), comp);
    appendField(cursor, labels.at(5), uncomp);
    appendField(cursor, labels.at(6), pkgEntry.md5);
    appendField(cursor, labels.at(7), ascEntry.md5);
    appendField(cursor, labels.at(8), pkgEntry.relativePath);
    appendDependencyField(cursor, labels.at(9), required);
    appendDependencyField(cursor, labels.at(10), conflicts);
    appendDependencyField(cursor, labels.at(11), suggests);

    if (!detailedDesc.trimmed().isEmpty()) {
        if (!infoText->document()->isEmpty()) {
            cursor.insertBlock();
            cursor.setBlockFormat(fieldFormat);
        }

        cursor.setCharFormat(boldFormat);
        cursor.insertText(tr("Package Description") + ":");
        cursor.setBlockFormat(descFormat);
        cursor.insertText(QString(QChar::LineSeparator));
        cursor.setCharFormat(normalFormat);

        QString flatDesc = detailedDesc;
        flatDesc.replace(QStringLiteral("\r\n"), QString(QChar::LineSeparator));
        flatDesc.replace(QStringLiteral("\n"), QString(QChar::LineSeparator));
        cursor.insertText(flatDesc);
    }

    currentLoadSignature = QStringLiteral("%1|%2|%3|%4")
            .arg(this->property("repoName").toString(), name, version, QString::number(row));
    fileList->header()->setSectionResizeMode(QHeaderView::Interactive);
    fileList->header()->setStretchLastSection(true);
    fileList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    fileList->clear();

    if (isInstalled) {
        pendingFiles = Packages::listPackageFiles(name, version);

        if (!pendingFiles.isEmpty()) {
            pendingFileIndex = 0;
            pendingMaxWidth = 0;
            QTimer::singleShot(0, this, &RepositoryTab::appendFileBatch);
        }
    }
}

/**
 * @brief Appends a single "label \t value" field as a new block in the info panel.
 * @param cursor Cursor positioned at the end of infoText's document; advanced past the inserted block.
 * @param label Bold field name shown before the tab stop (e.g. "Package Name:").
 * @param value Plain-text value shown after the tab stop; the field is skipped when this is blank.
 */
void RepositoryTab::appendField(QTextCursor cursor, const QString &label, const QString &value) const {
    if (value.trimmed().isEmpty()) return;

    if (!infoText->document()->isEmpty()) {
        cursor.insertBlock();
        cursor.setBlockFormat(fieldFormat);
    }

    cursor.setCharFormat(boldFormat);
    cursor.insertText(label);
    cursor.setCharFormat(normalFormat);
    cursor.insertText(QStringLiteral("\t") + value);
}

/**
 * @brief Appends a label followed by a comma-separated dependency list.
 * @param cursor Cursor positioned at the end of infoText's document; advanced past the inserted blocks.
 * @param label Bold field name shown before the first dependency (e.g. "Package Required:").
 * @param value Comma-separated list of dependency names; the field is skipped when this is blank.
 */
void RepositoryTab::appendDependencyField(QTextCursor cursor, const QString &label, const QString &value) const {
    if (value.trimmed().isEmpty()) return;
    if (!infoText->document()->isEmpty())
        cursor.insertBlock();

    cursor.setBlockFormat(fieldFormat);
    cursor.setCharFormat(boldFormat);
    cursor.insertText(label);

    const QStringList depList = value.split(u',', Qt::SkipEmptyParts);
    cursor.setCharFormat(normalFormat);
    cursor.insertText(QStringLiteral("\t") + depList.at(0).trimmed());

    if (depList.size() == 1) {
        cursor.setBlockFormat(fieldFormat);
    } else {
        QTextBlockFormat firstDepFormat = fieldFormat;
        firstDepFormat.setLineHeight(100, QTextBlockFormat::ProportionalHeight);
        cursor.setBlockFormat(firstDepFormat);
    }

    for (int i = 1; i < depList.size(); ++i) {
        cursor.insertBlock();

        if (i == depList.size() - 1)
            cursor.setBlockFormat(depLastFormat);
        else
            cursor.setBlockFormat(depMidFormat);

        cursor.setCharFormat(normalFormat);
        cursor.insertText(depList.at(i).trimmed());
    }
}

/**
 * @brief Fills the package model with the provided package list.
 * @param pkgs List containing the package information to display.
 */
void RepositoryTab::fillTable(const QList<PkgInfo> &pkgs) {
    proxyModel->setSourceModel(nullptr);
    packageTable->block();

    if (showCategories) {
        catList->clear();
        root = new QTreeWidgetItem(catList, QStringList(tr("Overview")));
        root->setExpanded(true);
    }

    const QString curRepo = this->property("repoName").toString();
    const bool isMultiple = curRepo == ALL_REPOSITORIES;

    auto statusOf = [&](const PkgInfo &p) -> RuleSt {
        return ruleStatuses.value({p.name, p.version, p.repoName}, RuleSt::Normal);
    };

    QHash<QString, QList<const PkgInfo *> > groupedPackages;
    groupedPackages.reserve(pkgs.size());

    for (const auto &pkg: pkgs) {
        const QString key = isMultiple ? pkg.repoName + u'|' + pkg.name : pkg.name;
        groupedPackages[key].append(&pkg);
    }

    QSet<QString> categories;
    QList<PackageRowData> rows;
    rows.reserve(groupedPackages.size());

    for (const QList<const PkgInfo *> &pkgVersions: std::as_const(groupedPackages)) {
        if (pkgVersions.isEmpty())
            continue;

        const PkgInfo *installedPkg{}, *metaPkg{}, *prioritizedPkg{}, *slackwarePkg{};

        for (const auto *pkg: pkgVersions) {
            if (!prioritizedPkg && statusOf(*pkg) == RuleSt::Prioritized)
                prioritizedPkg = pkg;
            if (pkg->isInstalled && !installedPkg)
                installedPkg = pkg;
            if (!pkg->isInstalled && !metaPkg)
                metaPkg = pkg;
            if (!slackwarePkg && pkg->repoName.compare(SLACK_OFICIAL) == 0)
                slackwarePkg = pkg;
        }

        if (!metaPkg)
            metaPkg = installedPkg ? installedPkg : pkgVersions.first();

        const PkgInfo &mPkg = *metaPkg;
        const PkgInfo &dPkg = prioritizedPkg
                                  ? *prioritizedPkg
                                  : installedPkg
                                        ? *installedPkg
                                        : *pkgVersions.first();
        const QString categorySource = slackwarePkg ? slackwarePkg->category : mPkg.category;

        const RuleSt dStatus = statusOf(dPkg);
        const bool isExcluded = dStatus == RuleSt::Excluded;
        const bool rowHasPriorityWinner = prioritizedPkg != nullptr;

        PackageRowData row;
        row.status = isExcluded
                         ? PkgStatus::Excluded
                         : dPkg.isInstalled
                               ? PkgStatus::Installed
                               : PkgStatus::Available;
        row.isPrioritized = rowHasPriorityWinner;
        row.name = dPkg.name;
        row.version = dPkg.version;
        row.description = mPkg.description;
        row.category = categorySource;
        row.isInstalled = dPkg.isInstalled;

        QString initialRepo = dPkg.repoName;
        if (dPkg.isInstalled) {
            for (const auto *p: pkgVersions) {
                if (!p->isInstalled && p->version == dPkg.version) {
                    initialRepo = p->repoName;
                    break;
                }
            }
        }
        row.repoName = initialRepo;

        row.allVersions.reserve(pkgVersions.size());
        row.pkgList.reserve(pkgVersions.size());

        for (const auto *v: pkgVersions) {
            row.allVersions.append(v->version);
            row.pkgList.append(*v);
        }

        rows.append(std::move(row));

        if (showCategories)
            categories.insert(categorySource);
    }

    if (showCategories) {
        QStringList sortedCats = categories.values();
        sortedCats.sort();
        for (const QString &cat: sortedCats)
            new QTreeWidgetItem(root, QStringList{cat});
    }

    packageModel->setRows(std::move(rows));
    proxyModel->setSourceModel(packageModel);
    packageTable->ublock();
    packageTable->sortByColumn(1, Qt::AscendingOrder);
    packageTable->resizeColumnToContents(0);
    packageTable->resizeColumnToContents(1);
    packageTable->resizeColumnToContents(2);

    rebuildPackageRowIndex();
}

/**
 * @brief Rebuilds the fast O(1) lookup indices for row positions.
 */
void RepositoryTab::rebuildPackageRowIndex() {
    packageRowIndex.clear();
    nameRowIndex.clear();

    const int count = packageModel->rowCount();
    packageRowIndex.reserve(count);
    nameRowIndex.reserve(count);

    for (int row = 0; row < count; ++row) {
        const auto &rowData = packageModel->rowAt(row);
        nameRowIndex.insert(rowData.name, row);
        packageRowIndex.insert(rowData.name + u'|' + rowData.version, row);
    }
}

/**
 * @brief Determines the priority ranking for choosing an update or install target repository.
 * @param repo Name or path of the repository to evaluate.
 * @param rowIsTestingOnly Flag indicating if the current row context is restricted exclusively to testing repositories.
 * @return Integer score (0–3) representing repository precedence (higher score indicates higher priority).
 */
int RepositoryTab::getRepoPriority(const QString &repo, const bool rowIsTestingOnly) const {
    if ((this->property("attachTesting").toBool() || rowIsTestingOnly) && repo.contains(SLACK_TESTING))
        return 3;
    if (repo.contains(SLACK_PATCHES))
        return 2;
    if (repo.compare(SLACK_OFICIAL) == 0)
        return 1;
    return 0;
}

/**
 * @brief Resolves candidate package records for installation or upgrade evaluation.
 * @param pkg Name of the target package to resolve.
 * @param fallback List of fallback package candidates used when primary resolution yields no match.
 * @param testing Flag indicating whether testing repository candidates should be considered.
 * @return List of candidate PkgInfo structures prioritized for installation or update.
 */
QList<PkgInfo> RepositoryTab::resolveCand(const QString &pkg, const QList<PkgInfo> &fallback,
                                          const bool testing) const {
    bool rowIsHierarchy = false;
    for (const auto &pkgName: fallback) {
        if (RepositoryTabUtils::isHierarchyRepo(pkgName.repoName)) {
            rowIsHierarchy = true;
            break;
        }
    }

    if (!rowIsHierarchy)
        return fallback;

    if (!globalByName)
        return fallback;

    const auto it = globalByName->find(pkg);
    const QList<PkgInfo> &global = (it != globalByName->end()) ? it.value() : fallback;

    QList<PkgInfo> hierarchyOnly;
    hierarchyOnly.reserve(global.size());
    for (const auto &pkgName: global) {
        if (RepositoryTabUtils::isHierarchyRepo(pkgName.repoName))
            hierarchyOnly.append(pkgName);
    }

    if (this->property("attachTesting").toBool() || testing)
        return hierarchyOnly;

    QList<PkgInfo> filtered;
    filtered.reserve(hierarchyOnly.size());
    for (const auto &pkgName: hierarchyOnly) {
        if (!pkgName.repoName.contains(SLACK_TESTING))
            filtered.append(pkgName);
    }
    return filtered;
}

/**
 * @brief Constructs a pending package descriptor with resolved mirror and download URLs.
 * @param pkg Name of the package.
 * @param ver Version string of the target package.
 * @param repo Name of the repository hosting the package.
 * @return Fully initialized PendingPkg structure containing package metadata and URLs.
 */
PendingPkg RepositoryTab::createPendingPackage(const QString &pkg, const QString &ver, const QString &repo) const {
    PendingPkg pending;
    pending.name = pkg;
    pending.version = ver;
    pending.repoName = repo;

    QString baseMirror = mirrorMap.value(repo, QString{});
    if (!baseMirror.isEmpty() && !baseMirror.endsWith(u'/'))
        baseMirror += u'/';

    const ChecksumEntry pkgEntry = packagesManager->getChecksumEntry(
        repo, pending.name, pending.version, FileFormat::Package);

    const int slashIdx = static_cast<int>(pkgEntry.relativePath.indexOf(u'/'));
    pending.category = slashIdx != -1 ? pkgEntry.relativePath.left(slashIdx) : QStringLiteral("Others");

    pending.fullDownloadUrl = baseMirror + pkgEntry.relativePath;
    pending.md5sum = pkgEntry.md5;

    const ChecksumEntry ascEntry = packagesManager->getChecksumEntry(
        repo, pending.name, pending.version, FileFormat::Asc);
    pending.hasAsc = !ascEntry.relativePath.isEmpty();
    pending.ascUrl = pending.hasAsc ? (baseMirror + ascEntry.relativePath) : QString{};
    pending.ascMd5sum = ascEntry.md5;

    return pending;
}

/**
 * @brief Resolves the best available update candidate for a given package among repository entries.
 * @param pkg Name of the package to resolve.
 * @param list List of candidate PkgInfo records associated with the package.
 * @param testing Flag indicating whether testing repository candidates are eligible.
 * @return PendingPkg structure representing the optimal update target.
 */
PendingPkg RepositoryTab::resolveBestUpdate(const QString &pkg, const QList<PkgInfo> &list, const bool testing) const {
    bool rowIsTestingOnly = !list.isEmpty();
    for (const auto &p: list) {
        if (!p.repoName.contains(SLACK_TESTING)) {
            rowIsTestingOnly = false;
            break;
        }
    }

    if (rowIsTestingOnly && !testing && !this->property("attachTesting").toBool())
        return PendingPkg{};

    const QList<PkgInfo> candidates = resolveCand(pkg, list, rowIsTestingOnly);

    const PkgInfo *installedCandidate = nullptr;
    for (const auto &pkgName: candidates) {
        if (pkgName.isInstalled) {
            installedCandidate = &pkgName;
            break;
        }
    }

    if (!installedCandidate && this->property("repoName").toString().compare(SLACK_OFICIAL) == 0) {
        if (globalByName) {
            if (const auto it = globalByName->find(pkg); it != globalByName->end()) {
                for (const auto &p: it.value()) {
                    if (!p.isInstalled || RepositoryTabUtils::isHierarchyRepo(p.repoName))
                        continue;

                    const RuleSt status = ruleStatuses.value({p.name, p.version, p.repoName}, RuleSt::Normal);
                    if (status == RuleSt::Excluded || status == RuleSt::Prioritized)
                        continue;

                    installedCandidate = &p;
                    break;
                }
            }
        }
    }

    if (!installedCandidate)
        return PendingPkg{};

    const PkgInfo *bestCand = nullptr;
    int maxPriority = -1;

    for (const auto &p: candidates) {
        if (p.isInstalled || ruleStatuses.value({p.name, p.version, p.repoName},
                                                RuleSt::Normal) == RuleSt::Excluded)
            continue;

        if (const int priority = getRepoPriority(p.repoName, rowIsTestingOnly); priority > maxPriority) {
            maxPriority = priority;
            bestCand = &p;
        }
    }

    if (bestCand) {
        const int installedPriority = getRepoPriority(installedCandidate->repoName, rowIsTestingOnly);
        const bool isHigherPriority = maxPriority > installedPriority;
        const bool isSamePriorityNewVersion =
                maxPriority == installedPriority && bestCand->version != installedCandidate->version;
        if (!isHigherPriority && !isSamePriorityNewVersion)
            bestCand = nullptr;
    }

    if (!bestCand)
        return PendingPkg{};

    PendingPkg pending = createPendingPackage(bestCand->name, bestCand->version, bestCand->repoName);
    pending.previousVersion = installedCandidate->version;
    return pending;
}

/**
 * @brief Scans every row currently in this tab and resolves an available update, if any, for each.
 */
QList<PendingPkg> RepositoryTab::collectAvailableUpdates() const {
    QList<PendingPkg> updates;
    const int count = packageModel->rowCount();
    updates.reserve(100);

    for (int row = 0; row < count; ++row) {
        const auto &rowData = packageModel->rowAt(row);
        if (rowData.pkgList.isEmpty())
            continue;

        if (PendingPkg update = resolveBestUpdate(rowData.name, rowData.pkgList, false); !update.name.isEmpty())
            updates.append(std::move(update));
    }

    return updates;
}

/**
 * @brief Lists every package available in the official hierarchy (Slackware/Patches always, Testing only if merged).
 */
QList<PendingPkg> RepositoryTab::collectNewInstalls() const {
    QList<PendingPkg> installs;
    if (!globalByName)
        return installs;

    QSet<QString> installedNames;
    const auto installed = packagesManager->getInstalledPackages();
    installedNames.reserve(installed.size());
    for (const auto &pkg: installed)
        installedNames.insert(pkg.name);

    const bool attachTestingEnabled = this->property("attachTesting").toBool();

    for (auto [pkgName, pkgList]: std::as_const(*globalByName).asKeyValueRange()) {
        if (installedNames.contains(pkgName))
            continue;

        const PkgInfo *bestCand = nullptr;
        int maxPriority = -1;

        for (const auto &pkg: pkgList) {
            if (pkg.isInstalled || !RepositoryTabUtils::isHierarchyRepo(pkg.repoName) ||
                (!attachTestingEnabled && pkg.repoName.contains(SLACK_TESTING)) ||
                ruleStatuses.value({pkg.name, pkg.version, pkg.repoName},
                                   RuleSt::Normal) == RuleSt::Excluded)
                continue;

            if (const int priority = getRepoPriority(pkg.repoName, false); priority > maxPriority) {
                maxPriority = priority;
                bestCand = &pkg;
            }
        }

        if (bestCand)
            installs.append(createPendingPackage(bestCand->name, bestCand->version, bestCand->repoName));
    }

    return installs;
}

/**
 * @brief Shows the context menu for the repository tab at the given position.
 * @param pos The local coordinates where the context menu should be displayed.
 */
void RepositoryTab::showContextMenu(const QPoint &pos) {
    if (packageTable->selectionModel()->selectedRows().isEmpty())
        return;

    ContextMenu menu(this);
    menu.addAction(installAction);
    menu.addAction(upgradeAction);
    menu.addAction(reinstallAction);
    // menu.addAction(rollbackAction); // TODO NO ACTIVE
    menu.addAction(removeAction);
    menu.addSeparator();
    // ContextMenu *adminMenu = menu.addContextMenu(tr("Administrative Actions"));
    // adminMenu->addAction(lockAction); // TODO NO ACTIVE
    // adminMenu->addAction(priorityAct); // TODO NO ACTIVE
    menu.addSeparator();
    menu.addAction(unselectAct);
    menu.exec(packageTable->viewport()->mapToGlobal(pos));
}

/**
 * @brief Runs @p cmd against every currently-selected row and emits the resulting batches.
 * @param cmd Which action to run.
 */
void RepositoryTab::executeCommand(const ActionType cmd) {
    const QModelIndexList selectedProxyIndexes = packageTable->selectionModel()->selectedRows();
    if (selectedProxyIndexes.isEmpty())
        return;

    QSet<int> selectedSourceRows;
    selectedSourceRows.reserve(selectedProxyIndexes.size());

    for (const QModelIndex &proxyIdx: selectedProxyIndexes)
        selectedSourceRows.insert(proxyModel->mapToSource(proxyIdx).row());

    const QString currentRepoContext = this->property("repoName").toString();

    QList<PendingPkg> installBatch, updateBatch, reinstallBatch, removeBatch, unselectBatch;
    QSet<QString> addedToInstall;

    for (const int row: selectedSourceRows) {
        if (row < 0 || row >= packageModel->rowCount())
            continue;

        const auto &rowData = packageModel->rowAt(row);
        if (rowData.status == PkgStatus::Excluded)
            continue;

        const QString pkg = rowData.name;
        const QString ver = rowData.version;
        QString repo = rowData.repoName;
        if (repo.isEmpty())
            repo = currentRepoContext;

        const bool isInstalled = rowData.isInstalled;
        const auto &pkgList = rowData.pkgList;

        bool rowIsTestingOnly = !pkgList.isEmpty();
        for (const auto &p: pkgList) {
            if (!p.repoName.contains(SLACK_TESTING)) {
                rowIsTestingOnly = false;
                break;
            }
        }

        if (cmd == ActionType::Unselect) {
            if (rowData.actionSymbol.isEmpty())
                continue;

            PendingPkg pending;
            pending.name = pkg;
            pending.version = !rowData.targetVersion.isEmpty() ? rowData.targetVersion : ver;
            pending.repoName = !rowData.targetRepo.isEmpty() ? rowData.targetRepo : repo;
            unselectBatch.append(std::move(pending));
            continue;
        }

        if (cmd == ActionType::Remove) {
            if (!isInstalled) continue;
            PendingPkg pending;
            pending.name = pkg;
            pending.version = ver;
            pending.repoName = repo;
            removeBatch.append(std::move(pending));
            continue;
        }

        if (cmd == ActionType::Reinstall) {
            if (!isInstalled) continue;
            if (packagesManager->getChecksumEntry(repo, pkg, ver, FileFormat::Package).relativePath.isEmpty())
                continue;

            reinstallBatch.append(createPendingPackage(pkg, ver, repo));
            continue;
        }

        if (cmd == ActionType::Install) {
            if (isInstalled) continue;

            const QList<PkgInfo> candidates = resolveCand(pkg, pkgList, rowIsTestingOnly);
            bool installedElsewhere = false;
            for (const auto &pkgName: candidates) {
                if (pkgName.isInstalled) {
                    installedElsewhere = true;
                    break;
                }
            }
            if (installedElsewhere)
                continue;

            const PkgInfo *bestCandidate = nullptr;
            int maxPriority = -1;

            for (const auto &pkgName: candidates) {
                if (!pkgName.isInstalled) {
                    if (const int priority = getRepoPriority(pkgName.repoName, rowIsTestingOnly);
                        priority > maxPriority) {
                        maxPriority = priority;
                        bestCandidate = &pkgName;
                    }
                }
            }

            const QString targetName = bestCandidate ? bestCandidate->name : pkg;
            const QString targetVersion = bestCandidate ? bestCandidate->version : ver;
            const QString targetRepo = bestCandidate ? bestCandidate->repoName : repo;

            addInstallWithDependencies(targetName, targetVersion, targetRepo, addedToInstall, installBatch);
            continue;
        }

        if (cmd == ActionType::Update) {
            if (PendingPkg update = resolveBestUpdate(pkg, pkgList, true); !update.name.isEmpty())
                updateBatch.append(std::move(update));
        }
    }

    if (!installBatch.isEmpty())
        emit packageActionRequested(installBatch, ActionType::Install);
    if (!updateBatch.isEmpty())
        emit packageActionRequested(updateBatch, ActionType::Update);
    if (!reinstallBatch.isEmpty())
        emit packageActionRequested(reinstallBatch, ActionType::Reinstall);
    if (!removeBatch.isEmpty())
        emit packageActionRequested(removeBatch, ActionType::Remove);
    if (!unselectBatch.isEmpty())
        emit packageUnselectRequested(unselectBatch);
}

/**
 * @brief Applies a status to many packages using a single view lock/unlock cycle.
 * @param pkgs The batch of packages to update.
 * @param st The pending action state (empty restores the default installed/available state).
 */
void RepositoryTab::updatePackageStatusBatch(const QList<PendingPkg> &pkgs, const PkgStatus st) const {
    if (pkgs.isEmpty())
        return;

    const bool attachTestingEnabled = this->property("attachTesting").toBool();
    const bool isUpdateRelated = st == PkgStatus::Reset || st == PkgStatus::PendingUpdate;

    packageTable->block();

    for (const auto &pkg: pkgs) {
        QList<int> candidateRows = packageRowIndex.values(pkg.name + u'|' + pkg.version);

        if (candidateRows.isEmpty())
            candidateRows = nameRowIndex.values(pkg.name);

        for (const int row: candidateRows) {
            if (row < 0 || row >= packageModel->rowCount())
                continue;

            auto &rowData = packageModel->rowAt(row);
            if (rowData.name != pkg.name)
                continue;

            const auto &pkgsInfo = rowData.pkgList;
            bool belongsToThisRow = pkgsInfo.isEmpty();
            for (const auto &p: pkgsInfo) {
                if (p.repoName == pkg.repoName) {
                    belongsToThisRow = true;
                    break;
                }
            }

            if (!belongsToThisRow && isUpdateRelated) {
                bool rowIsHierarchy = false;
                bool rowIsTestingOnly = !pkgsInfo.isEmpty();
                for (const auto &p: pkgsInfo) {
                    if (RepositoryTabUtils::isHierarchyRepo(p.repoName))
                        rowIsHierarchy = true;
                    if (!p.repoName.contains(SLACK_TESTING))
                        rowIsTestingOnly = false;
                }

                if (rowIsHierarchy) {
                    const bool pkgIsTesting = pkg.repoName.contains(SLACK_TESTING);
                    const bool pkgIsPatches = pkg.repoName.contains(SLACK_PATCHES);

                    if (attachTestingEnabled) {
                        if (pkgIsPatches || pkgIsTesting)
                            belongsToThisRow = true;
                    } else if (pkgIsTesting || (pkgIsPatches && !rowIsTestingOnly)) {
                        belongsToThisRow = true;
                    }
                }
            }

            if (!belongsToThisRow)
                continue;

            if (st == PkgStatus::Reset) {
                rowData.actionSymbol.clear();
                rowData.targetVersion.clear();
                rowData.targetRepo.clear();
                rowData.status = rowData.isInstalled ? PkgStatus::Installed : PkgStatus::Available;
            } else {
                rowData.actionSymbol = QString::number(static_cast<int>(st));
                rowData.targetVersion = pkg.version;
                rowData.targetRepo = pkg.repoName;
                rowData.status = st;
            }

            packageModel->notifyRowChanged(row);
        }
    }

    packageTable->ublock();
}

/**
 * @brief Re-renders the table using the current package list.
 */
void RepositoryTab::refreshTable() const {
    auto statusOf = [&](const QString &name, const QString &ver, const QString &repo) -> RuleSt {
        return ruleStatuses.value({name, ver, repo}, RuleSt::Normal);
    };

    const int count = packageModel->rowCount();
    for (int row = 0; row < count; ++row) {
        auto &r = packageModel->rowAt(row);
        bool rowHasPriorityWinner = false;

        for (const auto &pkg : r.pkgList) {
            if (statusOf(pkg.name, pkg.version, pkg.repoName) == RuleSt::Prioritized) {
                rowHasPriorityWinner = true;
                break;
            }
        }

        const RuleSt dStatus = statusOf(r.name, r.version, r.repoName);
        const bool isExcluded = (dStatus == RuleSt::Excluded);

        if (r.actionSymbol.isEmpty()) {
            r.status = isExcluded ? PkgStatus::Excluded
                     : r.isInstalled ? PkgStatus::Installed
                     : PkgStatus::Available;
        } else if (isExcluded) {
            r.status = PkgStatus::Excluded;
        }

        r.isPrioritized = rowHasPriorityWinner;
        packageModel->notifyRowChanged(row);
    }
}

/**
 * @brief Searches for a package that satisfies a specific dependency requirement.
 * @param depName The name of the dependency package to find.
 * @param parentRepoName The name of the repository associated with the parent package.
 * @return A PkgInfo object containing the package details if found, or an empty PkgInfo object otherwise.
 */
PkgInfo RepositoryTab::findDependencyCandidate(const QString &depName, const QString &parentRepoName) const {
    for (const auto &pkg: packagesManager->getInstalledPackages()) {
        if (pkg.name == depName)
            return pkg;
    }

    const auto available = packagesManager->getAvailablePackages();
    if (const auto it = available.find(parentRepoName); it != available.end()) {
        for (const auto &pkg: it.value().packages) {
            if (pkg.name == depName)
                return pkg;
        }
    }

    return PkgInfo{};
}

/**
 * @brief Recursively schedules a package and all its unresolved dependencies for installation.
 * @param pkgName The name of the package to be installed.
 * @param version The version of the package.
 * @param repoName The repository where the package is located.
 * @param addedToInstall A set tracking packages already processed to prevent duplicate processing or infinite loops.
 * @param installBatch The output list where scheduled pending packages are appended.
 * @param parentName The name of the package that triggered this dependency resolution (empty for the root package).
 */
void RepositoryTab::addInstallWithDependencies(const QString &pkgName, const QString &version,
                                               const QString &repoName, QSet<QString> &addedToInstall,
                                               QList<PendingPkg> &installBatch, const QString &parentName) const {
    if (addedToInstall.contains(pkgName))
        return;

    addedToInstall.insert(pkgName);

    PendingPkg pending = createPendingPackage(pkgName, version, repoName);
    pending.dependencyOf = parentName;
    installBatch.append(std::move(pending));

    auto info = packagesManager->getPackageInfo(repoName, pkgName, version);
    if (info.name.isEmpty())
        info = packagesManager->getPackageInfo({}, pkgName, version);

    if (!info.required.trimmed().isEmpty()) {
        for (const QString &rawDep: info.required.split(u',', Qt::SkipEmptyParts)) {
            QString depName = rawDep.trimmed();
            if (const int spaceIdx = static_cast<int>(depName.indexOf(QLatin1Char(' '))); spaceIdx != -1)
                depName = depName.left(spaceIdx).trimmed();

            if (depName.isEmpty() || addedToInstall.contains(depName))
                continue;

            const PkgInfo depCandidate = findDependencyCandidate(depName, repoName);
            if (depCandidate.name.isEmpty() || depCandidate.isInstalled)
                continue;

            addInstallWithDependencies(depCandidate.name, depCandidate.version, depCandidate.repoName,
                                       addedToInstall, installBatch, pkgName);
        }
    }
}
