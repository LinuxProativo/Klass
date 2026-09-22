/**
 * @file RepositoryTab.cpp
 * @brief Interface providing a split-pane layout for categories, package lists, and others tabs.
 */

#include <QFile>
#include <QHeaderView>
#include <QMenu>

#include <CenterDelegate.hpp>
#include <ContextMenu.hpp>
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
    debug = new Debug::Debug();

    catList = new TreeWidget();
    catList->setHeaderLabel(tr("Categories"));
    catList->setMaximumWidth(300);
    catList->setVisible(isvisible);
    connect(catList, &TreeWidget::itemClicked, this, &RepositoryTab::onCategoryItemClicked);

    packageModel = new QStandardItemModel(0, 4, this);
    packageModel->setHorizontalHeaderLabels({tr("Status"), tr("Name"), tr("Version"), tr("Description")});

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
    proxyModel->setCategoryFilter(item->text(0));
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

    if (const QVariant varPkgs = index.data(Qt::UserRole + 4); varPkgs.typeId() == qMetaTypeId<QList<PkgInfo> >()) {
        if (const auto pkgList = varPkgs.value<QList<PkgInfo> >(); vIndex >= 0 && vIndex < pkgList.size()) {
            const PkgInfo &nPkg = pkgList[vIndex];
            const PkgInfo *metaPkgPtr{};
            const PkgInfo *slackwarePkgPtr{};
            bool rowHasPriorityWinner = false;

            for (const auto &pkg: pkgList) {
                if (!pkg.isInstalled && !metaPkgPtr)
                    metaPkgPtr = &pkg;
                if (!slackwarePkgPtr && pkg.repoName.compare(SLACK_OFICIAL) == 0)
                    slackwarePkgPtr = &pkg;
                if (ruleStatuses.value({pkg.name, pkg.version, pkg.repoName},
                                       RuleSt::Normal) == RuleSt::Prioritized) {
                    rowHasPriorityWinner = true;
                }
            }

            const PkgInfo &metaPkg = metaPkgPtr ? *metaPkgPtr : nPkg;
            const QString categorySource = slackwarePkgPtr ? slackwarePkgPtr->category : metaPkg.category;
            const RuleSt status = ruleStatuses.value({nPkg.name, nPkg.version, nPkg.repoName}, RuleSt::Normal);
            auto *statusItem = packageModel->item(row, 0);
            const QString actionSymbol = statusItem ? statusItem->data(Qt::UserRole + 5).toString() : QString{};
            const QString targetVersion = statusItem ? statusItem->data(Qt::UserRole + 6).toString() : QString{};

            PkgStatus packageState;
            if (status == RuleSt::Excluded) {
                packageState = PkgStatus::Excluded;
            } else if (!actionSymbol.isEmpty() && nPkg.version == targetVersion) {
                if (actionSymbol == QStringLiteral("update"))
                    packageState = PkgStatus::PendingUpdate;
                else
                    packageState = nPkg.isInstalled ? PkgStatus::PendingReinstall : PkgStatus::PendingInstall;
            } else if (nPkg.isInstalled) {
                packageState = PkgStatus::Installed;
            } else {
                packageState = PkgStatus::Available;
            }

            if (statusItem) {
                RepositoryTabUtils::setPackageStatus(statusItem, packageState);
                statusItem->setData(rowHasPriorityWinner, Qt::UserRole + 3);
            }

            packageModel->item(row, 3)->setText(metaPkg.description);

            QString targetRepoName = nPkg.repoName;
            if (nPkg.isInstalled) {
                for (const auto &pkg: pkgList) {
                    if (!pkg.isInstalled && pkg.version == nPkg.version) {
                        targetRepoName = pkg.repoName;
                        break;
                    }
                }
            }

            auto *nameItem = packageModel->item(row, 1);
            nameItem->setData(categorySource, Qt::UserRole);
            nameItem->setData(nPkg.isInstalled, Qt::UserRole + 1);
            nameItem->setData(targetRepoName, Qt::UserRole + 2);

            debug->msg("Package version changed to", "RepositoryTab", {Debug::Cyan, nPkg.version});
        }
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
    debug->msg("Processing file batch for signature", "RepositoryTab", {Debug::Blue, localSignature});

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
    debug->msg("Batch processing completed for signature", "RepositoryTab", {Debug::LightGreen, localSignature});
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

    QStandardItem *statusItem = packageModel->item(row, 0);
    QStandardItem *nameItem = packageModel->item(row, 1);
    QStandardItem *versionItem = packageModel->item(row, 2);

    if (!nameItem || !versionItem || !statusItem) return;

    QString repoName = nameItem->data(Qt::UserRole + 2).toString();
    if (repoName.isEmpty())
        repoName = this->property("repoName").toString();
    debug->msg("Active repository context", "RepositoryTab", {Debug::Violet, repoName});

    if (repoName == SLACK_OTHERS)
        repoName.clear();

    const QString name = nameItem->text();
    const QString version = versionItem->text();
    const bool isInstalled = nameItem->data(Qt::UserRole + 1).toBool();
    auto info = packagesManager->getPackageInfo(repoName, name, version);
    debug->msg("Selected package target", "RepositoryTab", {Debug::Cyan, QString("%1-%2").arg(name, version)});

    if (info.name.isEmpty() && isInstalled && !repoName.isEmpty())
        info = packagesManager->getPackageInfo({}, name, version);

    const QString detailedDesc = info.detailedDesc;
    const QString comp = info.compressedSize;
    const QString uncomp = info.uncompressedSize;
    const QString required = info.required;
    const QString conflicts = info.conflicts;
    const QString suggests = info.suggests;
    const QString mirrorUrl = mirrorMap.value(repoName, QString{});
    ChecksumEntry pkgEntry = packagesManager->getChecksumEntry(repoName, name, version, FileFormat::Package);
    ChecksumEntry ascEntry = packagesManager->getChecksumEntry(repoName, name, version, FileFormat::Asc);

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

    const QStringList depList = value.split(QStringLiteral(","));
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
    packages = pkgs;
    proxyModel->setSourceModel(nullptr);
    packageTable->block();
    packageModel->removeRows(0, packageModel->rowCount());

    if (showCategories) {
        catList->clear();
        root = new QTreeWidgetItem(catList, QStringList{QStringLiteral("Overview")});
        root->setExpanded(true);
    }

    const QString curRepo = this->property("repoName").toString();
    const bool isMultiple = (curRepo == QLatin1String("Multiple"));

    auto statusOf = [&](const PkgInfo &p) -> RuleSt {
        return ruleStatuses.value({p.name, p.version, p.repoName}, RuleSt::Normal);
    };

    QHash<std::pair<QString, QString>, QList<PkgInfo> > groupedPackages;
    groupedPackages.reserve(pkgs.size());

    for (const auto &pkg: pkgs) {
        const QString keyRepo = isMultiple ? pkg.repoName : QString{};
        groupedPackages[{keyRepo, pkg.name}].append(pkg);
    }

    QSet<QString> categories;
    QStandardItem *rootItem = packageModel->invisibleRootItem();

    for (auto it = groupedPackages.begin(); it != groupedPackages.end(); ++it) {
        const QList<PkgInfo> &pkgVersions = it.value();
        if (pkgVersions.isEmpty())
            continue;

        const PkgInfo *installedPkg{};
        const PkgInfo *metaPkg{};
        const PkgInfo *prioritizedPkg{};
        const PkgInfo *slackwarePkg{};

        for (const auto &pkg: pkgVersions) {
            if (!prioritizedPkg && statusOf(pkg) == RuleSt::Prioritized)
                prioritizedPkg = &pkg;
            if (pkg.isInstalled && !installedPkg)
                installedPkg = &pkg;
            if (!pkg.isInstalled && !metaPkg)
                metaPkg = &pkg;
            if (!slackwarePkg && pkg.repoName.compare(SLACK_OFICIAL) == 0)
                slackwarePkg = &pkg;
        }

        if (!metaPkg)
            metaPkg = installedPkg ? installedPkg : &pkgVersions.first();

        const PkgInfo &mPkg = *metaPkg;
        const PkgInfo &dPkg = prioritizedPkg
                                  ? *prioritizedPkg
                                  : installedPkg
                                        ? *installedPkg
                                        : pkgVersions.first();
        const QString categorySource = slackwarePkg ? slackwarePkg->category : mPkg.category;

        const RuleSt dStatus = statusOf(dPkg);
        const bool isExcluded = (dStatus == RuleSt::Excluded);
        const bool rowHasPriorityWinner = (prioritizedPkg != nullptr);

        auto *statusItem = new QStandardItem(); // NOLINT
        RepositoryTabUtils::setPackageStatus(statusItem, isExcluded
                                                             ? PkgStatus::Excluded
                                                             : dPkg.isInstalled
                                                                   ? PkgStatus::Installed
                                                                   : PkgStatus::Available);
        statusItem->setData(rowHasPriorityWinner, Qt::UserRole + 3);

        auto *nameItem = new QStandardItem(dPkg.name); // NOLINT
        nameItem->setData(categorySource, Qt::UserRole);
        nameItem->setData(dPkg.isInstalled, Qt::UserRole + 1);
        QString initialRepo = dPkg.repoName;

        if (dPkg.isInstalled) {
            for (const auto &p: pkgVersions) {
                if (!p.isInstalled && p.version == dPkg.version) {
                    initialRepo = p.repoName;
                    break;
                }
            }
        }
        nameItem->setData(initialRepo, Qt::UserRole + 2);

        QStringList allVersions;
        allVersions.reserve(pkgVersions.size());
        for (const auto &v: pkgVersions)
            allVersions.append(v.version);

        auto *versionItem = new QStandardItem(dPkg.version); // NOLINT
        versionItem->setData(allVersions, Qt::UserRole + 3);
        versionItem->setData(QVariant::fromValue(pkgVersions), Qt::UserRole + 4);

        auto *descItem = new QStandardItem(mPkg.description); // NOLINT
        rootItem->appendRow({statusItem, nameItem, versionItem, descItem});

        if (showCategories)
            categories.insert(categorySource);
    }

    if (showCategories) {
        QStringList sortedCats = categories.values();
        sortedCats.sort();
        for (const QString &cat: sortedCats)
            new QTreeWidgetItem(root, QStringList{cat});
    }

    proxyModel->setSourceModel(packageModel);
    packageTable->ublock();
    packageTable->sortByColumn(1, Qt::AscendingOrder);
    packageTable->resizeColumnToContents(0);
    packageTable->resizeColumnToContents(1);
    packageTable->resizeColumnToContents(2);

    rebuildPackageRowIndex();
}

/**
 * @brief Rebuilds the cross-repository name -> package-list index used by the context menus Install/Upgrade actions.
 * @param all The full package list (all repos + orphaned/uncategorized entries).
 */
void RepositoryTab::setGlobalPackages(const QList<PkgInfo> &all) {
    globalByName.clear();
    globalByName.reserve(all.size());
    for (const auto &pkg: all)
        globalByName[pkg.name].append(pkg);
}

/**
 * @brief Rebuilds the name|version -> row lookup index used by updatePackageStatus/Batch.
 */
void RepositoryTab::rebuildPackageRowIndex() {
    packageRowIndex.clear();
    packageRowIndex.reserve(packageModel->rowCount());

    for (int row = 0; row < packageModel->rowCount(); ++row) {
        const QStandardItem *nameItem = packageModel->item(row, 1);
        const QStandardItem *versionItem = packageModel->item(row, 2);
        if (!nameItem || !versionItem) continue;
        packageRowIndex.insert({nameItem->text(), versionItem->text()}, row);
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

    const QList<PkgInfo> global = globalByName.value(pkg, fallback);

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
    pending.category = (slashIdx != -1) ? pkgEntry.relativePath.left(slashIdx) : QStringLiteral("Others");

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
        for (const QList<PkgInfo> global = globalByName.value(pkg); const auto &p: global) {
            if (!p.isInstalled || RepositoryTabUtils::isHierarchyRepo(p.repoName))
                continue;

            const RuleSt status = ruleStatuses.value({p.name, p.version, p.repoName}, RuleSt::Normal);
            if (status == RuleSt::Excluded || status == RuleSt::Prioritized)
                continue;

            installedCandidate = &p;
            break;
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

    for (int row = 0; row < packageModel->rowCount(); ++row) {
        const QStandardItem *nameItem = packageModel->item(row, 1);
        const QStandardItem *versionItem = packageModel->item(row, 2);
        if (!nameItem || !versionItem)
            continue;

        QList<PkgInfo> pkgList;
        if (QVariant varPkgs = versionItem->data(Qt::UserRole + 4); varPkgs.canConvert<QList<PkgInfo> >())
            pkgList = varPkgs.value<QList<PkgInfo> >();

        if (pkgList.isEmpty())
            continue;

        if (PendingPkg update = resolveBestUpdate(nameItem->text(), pkgList, false); !update.name.isEmpty())
            updates.append(update);
    }

    return updates;
}

/**
 * @brief Lists every package available in the official hierarchy (Slackware/Patches always, Testing only if merged).
 */
QList<PendingPkg> RepositoryTab::collectNewInstalls() const {
    QList<PendingPkg> installs{};
    QSet<QString> installedNames{};

    for (const auto &pkg: packagesManager->getInstalledPackages())
        installedNames.insert(pkg.name);

    const bool attachTestingEnabled = this->property("attachTesting").toBool();

    for (auto it = globalByName.constBegin(); it != globalByName.constEnd(); ++it) {
        if (const QString &pkgName = it.key(); installedNames.contains(pkgName))
            continue;

        const PkgInfo *bestCand = nullptr;
        int maxPriority = -1;

        for (const auto &pkg: it.value()) {
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
 * @brief Runs @p cmd against every currently-selected row, resolving each package via the same
 *        hierarchy-aware logic used everywhere else, and emits the resulting batches.
 * @param cmd Which action to run.
 */
void RepositoryTab::executeCommand(const ActionType cmd) {
    QModelIndexList selectedProxyIndexes = packageTable->selectionModel()->selectedRows();
    if (selectedProxyIndexes.isEmpty())
        return;

    QSet<int> selectedSourceRows;
    for (const QModelIndex &proxyIdx: selectedProxyIndexes) {
        QModelIndex srcIdx = proxyModel->mapToSource(proxyIdx);
        int row = srcIdx.row();
        selectedSourceRows.insert(row);
    }

    const QString currentRepoContext = this->property("repoName").toString();

    QList<PendingPkg> installBatch;
    QList<PendingPkg> updateBatch;
    QList<PendingPkg> reinstallBatch;
    QList<PendingPkg> removeBatch;
    QList<PendingPkg> unselectBatch;
    QSet<QString> addedToInstall;

    for (int row: selectedSourceRows) {
        QStandardItem *statusItem = packageModel->item(row, 0);
        QStandardItem *nameItem = packageModel->item(row, 1);
        QStandardItem *versionItem = packageModel->item(row, 2);
        if (!statusItem || !nameItem || !versionItem || RepositoryTabUtils::isExcluded(statusItem))
            continue;

        const QString pkg = nameItem->text();
        const QString ver = versionItem->text();
        QString repo = nameItem->data(Qt::UserRole + 2).toString();

        if (repo.isEmpty())
            repo = currentRepoContext;

        const bool isInstalled = nameItem->data(Qt::UserRole + 1).toBool();

        QList<PkgInfo> pkgList;
        if (QVariant varPkgs = versionItem->data(Qt::UserRole + 4); varPkgs.canConvert<QList<PkgInfo> >())
            pkgList = varPkgs.value<QList<PkgInfo> >();

        bool rowIsTestingOnly = !pkgList.isEmpty();
        for (const auto &p: pkgList) {
            if (!p.repoName.contains(SLACK_TESTING)) {
                rowIsTestingOnly = false;
                break;
            }
        }

        if (cmd == ActionType::Unselect) {
            if (statusItem->data(Qt::UserRole + 5).toString().isEmpty())
                continue;

            const QString pendingTargetVersion = statusItem->data(Qt::UserRole + 6).toString();
            const QString pendingTargetRepo = statusItem->data(Qt::UserRole + 7).toString();

            PendingPkg pending;
            pending.name = pkg;
            pending.version = !pendingTargetVersion.isEmpty() ? pendingTargetVersion : ver;
            pending.repoName = !pendingTargetRepo.isEmpty() ? pendingTargetRepo : repo;
            unselectBatch.append(pending);
            continue;
        }

        if (cmd == ActionType::Remove) {
            if (!isInstalled) continue;
            PendingPkg pending;
            pending.name = pkg;
            pending.version = ver;
            pending.repoName = repo;
            removeBatch.append(pending);
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
                    if (int priority = getRepoPriority(pkgName.repoName, rowIsTestingOnly); priority > maxPriority) {
                        maxPriority = priority;
                        bestCandidate = &pkgName;
                    }
                }
            }

            QString targetName = bestCandidate ? bestCandidate->name : pkg;
            QString targetVersion = bestCandidate ? bestCandidate->version : ver;
            QString targetRepo = bestCandidate ? bestCandidate->repoName : repo;

            addInstallWithDependencies(targetName, targetVersion, targetRepo, addedToInstall, installBatch);
            continue;
        }

        if (cmd == ActionType::Update) {
            if (PendingPkg update = resolveBestUpdate(pkg, pkgList, true); !update.name.isEmpty())
                updateBatch.append(update);
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
        QList<int> candidateRows = packageRowIndex.values({pkg.name, pkg.version});
        if (candidateRows.isEmpty()) {
            for (int row = 0; row < packageModel->rowCount(); ++row) {
                if (const QStandardItem *item = packageModel->item(row, 1); item && item->text() == pkg.name)
                    candidateRows.append(row);
            }
        }

        for (const int row: candidateRows) {
            const QStandardItem *nameItem = packageModel->item(row, 1);
            const QStandardItem *versionItem = packageModel->item(row, 2);
            if (!nameItem || !versionItem || nameItem->text() != pkg.name)
                continue;

            const auto pkgsInfo = versionItem->data(Qt::UserRole + 4).value<QList<PkgInfo> >();
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

            QStandardItem *statusItem = packageModel->item(row, 0);
            if (!statusItem) continue;

            if (st == PkgStatus::Reset) {
                statusItem->setData(QString{}, Qt::UserRole + 5);
                statusItem->setData(QString{}, Qt::UserRole + 6);
                statusItem->setData(QString{}, Qt::UserRole + 7);

                const bool isInstalled = nameItem->data(Qt::UserRole + 1).toBool();
                RepositoryTabUtils::setPackageStatus(
                    statusItem, isInstalled ? PkgStatus::Installed : PkgStatus::Available);
            } else {
                statusItem->setData(static_cast<int>(st), Qt::UserRole + 5);
                statusItem->setData(pkg.version, Qt::UserRole + 6);
                statusItem->setData(pkg.repoName, Qt::UserRole + 7);
                RepositoryTabUtils::setPackageStatus(statusItem, st);
            }
        }
    }

    packageTable->ublock();
}

/**
 * @brief Re-renders the table using the current package list and the rule statuses last supplied.
 */
void RepositoryTab::refreshTable() {
    if (!packages.isEmpty())
        fillTable(packages);
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

    if (const auto available = packagesManager->getAvailablePackages(); available.contains(parentRepoName)) {
        for (const auto &pkg: available.value(parentRepoName).packages) {
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
    installBatch.append(pending);

    auto info = packagesManager->getPackageInfo(repoName, pkgName, version);
    if (info.name.isEmpty())
        info = packagesManager->getPackageInfo({}, pkgName, version);

    if (!info.required.trimmed().isEmpty()) {
        for (const QString &rawDep: info.required.split(QStringLiteral(","), Qt::SkipEmptyParts)) {
            QString depName = rawDep.trimmed();
            if (int spaceIdx = static_cast<int>(depName.indexOf(QLatin1Char(' '))); spaceIdx != -1)
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
