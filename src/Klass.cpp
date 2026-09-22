/**
 * @file Klass.cpp
 * @brief This file serves as the main application interface, managing the window layout and components.
 */

#include <QLineEdit>
#include <QMessageBox>
#include <QStandardPaths>

#include <Klass.hpp>
#include <Packages.hpp>
#include <RepositoryTab.hpp>
#include <SlackwareDefines.hpp>

/**
 * @brief Constructs the main application window and initializes its components.
 * @param parent Pointer to the parent widget.
 */
Klass::Klass(QWidget *parent) : QWidget(parent) {
    this->setWindowTitle(tr("Klass Package Manager"));
    this->setWindowIcon(QIcon(DefaultPath().defaultPath("klass.png")));
    this->setAttribute(Qt::WA_StaticContents);

    terminalDialog = new Terminal(this);
    helper = new HelperController(terminalDialog, this);
    connect(helper, &HelperController::helperReady, this, &Klass::onHelperReady);
    connect(helper, &HelperController::updateFinished, this, &Klass::onUpdateFinished);
    connect(helper, &HelperController::updateRules, this, &Klass::reloadRules);
    connect(helper, &HelperController::transactionFinished, this, &Klass::onTransactionFinished);

    settingsManager = new SettingsManager(this);
    tray = new SysTray(this, helper->socket(), settingsManager);
    packagesManager = new Packages();

    about = new About(this);
    debug = new Debug::Debug();
    logView = new ChangeLog(this);
    installDialog = new InstallPackage::Install(this);

    rulesManagerDialog = new RulesManagerDialog(this);
    rulesManagerDialog->loadRules(RulesManager::loadRules());
    connect(rulesManagerDialog, &RulesManagerDialog::ruleAdded, this, &Klass::handleAddRuleRequested);
    connect(rulesManagerDialog, &RulesManagerDialog::ruleRemoved, this, &Klass::handleDeleteRulesRequested);
    connect(rulesManagerDialog, &RulesManagerDialog::ruleEdited, this, &Klass::handleEditRuleRequested);

    settingsDialog = new SettingsDialog(this);
    connect(settingsDialog, &SettingsDialog::settingsChanged, this, &Klass::loadRepositoryTabs);

    summaryDialog = new SummaryDialog(this);
    connect(summaryDialog, &QDialog::accepted, [this] { helper->setupHelper(PendingAction::ProcessTransaction); });

    mirrorManager = new MirrorManager(this);
    connect(mirrorManager, &MirrorManager::mirrorChangeRequested, this, &Klass::handleMirrorChangeRequested);
    connect(mirrorManager, &MirrorManager::reposEnable, this, &Klass::handleReposEnable);
    connect(mirrorManager, &MirrorManager::reposRemoved, this, &Klass::handleReposRemoved);

    searchBar = new QLineEdit(this);
    searchBar->setMinimumHeight(30);
    searchBar->setPlaceholderText(" " + tr("Search packages") + "...");
    connect(searchBar, &QLineEdit::textChanged, this, &Klass::onSearchTextChanged);

    auto *resetButton = new Buttons(Icon::Id::Reset, tr("Clear Search"), 28);
    connect(resetButton, &Buttons::clicked, searchBar, &QLineEdit::clear);

    searchDebounceTimer = new QTimer(this);
    searchDebounceTimer->setSingleShot(true);
    searchDebounceTimer->setInterval(250);
    connect(searchDebounceTimer, &QTimer::timeout, this, &Klass::applySearchFilterToAllTabs);

    btnAdd = new Buttons(Icon::Id::Add, tr("Add/Manage Slackware Mirrors"));
    connect(btnAdd, &Buttons::clicked, mirrorManager, &MirrorManager::show);

    btnException = new Buttons(Icon::Id::Exception, tr("Manage Priorities/Exception Rules"));
    connect(btnException, &Buttons::clicked, rulesManagerDialog, &RulesManagerDialog::show);

    btnUpdate = new Buttons(Icon::Id::Update, tr("Update Database"));
    connect(btnUpdate, &Buttons::clicked, this, &Klass::onUpdate);

    btnUp = new Buttons(Icon::Id::Up, tr("Select Updates"));
    connect(btnUp, &Buttons::clicked, this, &Klass::onSelectUpdates);

    btnApply = new Buttons(Icon::Id::Apply, tr("Apply Updates"));
    connect(btnApply, &Buttons::clicked, this, &Klass::onApplyTransaction);

    btnSettings = new Buttons(Icon::Id::Settings, tr("Settings"));
    connect(btnSettings, &Buttons::clicked, settingsDialog, &SettingsDialog::show);

    btnChangelog = new Buttons(Icon::Id::Changelog, tr("View Changelog"));
    connect(btnChangelog, &Buttons::clicked, logView, &ChangeLog::show);

    btnAbout = new Buttons(Icon::Id::About, tr("About"));
    connect(btnAbout, &Buttons::clicked, about, &About::show);

    searchLayout = new QHBoxLayout(searchBar);
    searchLayout->addWidget(resetButton, 0, Qt::AlignRight | Qt::AlignVCenter);
    searchLayout->setContentsMargins(0, 0, 2, 0);
    searchBar->setLayout(searchLayout);

    toolBarWidget = new QWidget(this);
    toolBarLayout = new QHBoxLayout(toolBarWidget);
    toolBarLayout->setContentsMargins(0, 0, 0, 0);
    toolBarLayout->setSpacing(0);
    toolBarLayout->addWidget(btnAdd);
    toolBarLayout->addWidget(btnException);
    toolBarLayout->addSpacing(15);
    toolBarLayout->addWidget(btnUpdate);
    toolBarLayout->addWidget(btnUp);
    toolBarLayout->addWidget(btnApply);
    toolBarLayout->addSpacing(15);
    toolBarLayout->addWidget(searchBar);
    toolBarLayout->addSpacing(15);
    toolBarLayout->addWidget(btnSettings);
    toolBarLayout->addWidget(btnChangelog);
    toolBarLayout->addWidget(btnAbout);

    tabWidget = new QTabWidget(this);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(toolBarWidget);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(tabWidget);
}

/**
 * @brief Propagates the search bar text to all active repository tabs.
 */
void Klass::applySearchFilterToAllTabs() const {
    const QString filterText = searchBar->text().trimmed();

    for (int i = 0; i < tabWidget->count(); ++i) {
        if (const auto *tab = qobject_cast<RepositoryTab *>(tabWidget->widget(i)))
            tab->setSearchFilter(filterText);
    }
}

/**
 * @brief Connects signal-slot relationships for a newly created repository tab.
 * @param tab Pointer to the RepositoryTab instance to configure.
 */
void Klass::connectTabSignals(const RepositoryTab *tab) const {
    connect(tab, &RepositoryTab::packageActionRequested, this, &Klass::handlePackageAction);
    connect(tab, &RepositoryTab::packageUnselectRequested, this, &Klass::handlePackageUnselectBatch);
}

/**
 * @brief Handles incoming package action requests from repository tabs.
 * @param pkgs The PendingPackage struct containing packages metadata.
 * @param type The action type requested (ActionType::Install, ActionType::Reinstall, ActionType::Remove).
 */
void Klass::handlePackageAction(const QList<PendingPkg> &pkgs, const ActionType type) {
    if (pkgs.isEmpty()) return;
    handlePackageUnselectBatch(pkgs);

    PkgStatus status;
    if (type == ActionType::Install) {
        pendingInstalls.append(pkgs);
        status = PkgStatus::PendingInstall;
    } else if (type == ActionType::Update) {
        pendingUpdates.append(pkgs);
        status = PkgStatus::PendingUpdate;
    } else if (type == ActionType::Reinstall) {
        pendingReinstalls.append(pkgs);
        status = PkgStatus::PendingReinstall;
    } else if (type == ActionType::Remove) {
        pendingRemoves.append(pkgs);
        status = PkgStatus::PendingRemove;
    } else return;

    notifyAllTabsBatch(pkgs, status);
}

/**
 * @brief Resolves and queues every available update across all open tabs, deduplicated by package name.
 */
void Klass::onSelectUpdates() {
    QList<PendingPkg> allUpdates{}, allNewInstalls{};
    QSet<QString> seenUpdates{}, seenInstalls{};

    for (int i = 0; i < tabWidget->count(); ++i) {
        if (const auto *tab = qobject_cast<RepositoryTab *>(tabWidget->widget(i))) {
            for (const auto &update: tab->collectAvailableUpdates()) {
                if (!seenUpdates.contains(update.name)) {
                    seenUpdates.insert(update.name);
                    allUpdates.append(update);
                }
            }
            for (const auto &install: tab->collectNewInstalls()) {
                if (!seenInstalls.contains(install.name)) {
                    seenInstalls.insert(install.name);
                    allNewInstalls.append(install);
                }
            }
        }
    }

    handlePackageAction(allUpdates, ActionType::Update);
    if (!allNewInstalls.isEmpty())
        handlePackageAction(allNewInstalls, ActionType::Install);
}

/**
 * @brief Populates the transaction summary from the current pending queues.
 */
void Klass::onApplyTransaction() const {
    summaryDialog->loadTransaction(pendingInstalls, pendingUpdates, pendingReinstalls, pendingRemoves);
    summaryDialog->show();
}

/**
 * @brief Removes a package from all pending action queues and restores its default status icon.
 * @param pkgs The PendingPackage struct identifying the package to unselect.
 */
void Klass::handlePackageUnselectBatch(const QList<PendingPkg> &pkgs) {
    if (pkgs.isEmpty()) return;

    QSet<std::pair<QString, QString> > toRemove;
    toRemove.reserve(pkgs.size());
    for (const auto &pkg: pkgs)
        toRemove.insert({pkg.name, pkg.version});

    auto removeByDetails = [&toRemove](QList<PendingPkg> &list) {
        list.removeIf([&toRemove](const PendingPkg &p) {
            return toRemove.contains({p.name, p.version});
        });
    };

    removeByDetails(pendingInstalls);
    removeByDetails(pendingUpdates);
    removeByDetails(pendingReinstalls);
    removeByDetails(pendingRemoves);
    notifyAllTabsBatch(pkgs, PkgStatus::Reset);
}

/**
 * @brief Propagates a status icon change for a batch of packages to all active repository tabs.
 * @param pkgs The batch of packages to update.
 * @param status The icon symbol to set, or reset status to restore default status.
 */
void Klass::notifyAllTabsBatch(const QList<PendingPkg> &pkgs, const PkgStatus status) const {
    if (pkgs.isEmpty()) return;

    for (int i = 0; i < tabWidget->count(); ++i) {
        if (const auto *tab = qobject_cast<RepositoryTab *>(tabWidget->widget(i)))
            tab->updatePackageStatusBatch(pkgs, status);
    }
}

/**
 * @brief Reloads and updates exception and priority rules across active tabs without rebuilding package trees.
 */
void Klass::reloadRules() const {
    const QList<RuleEntry> updatedRules = RulesManager::loadRules();
    const QHash<PkgKey, RuleSt> statuses = RulesManager::resolveStatuses(allPackages, updatedRules);

    if (rulesManagerDialog)
        rulesManagerDialog->loadRules(updatedRules);

    for (int i = 0; i < tabWidget->count(); ++i) {
        if (auto *tab = qobject_cast<RepositoryTab *>(tabWidget->widget(i))) {
            tab->setRuleStatuses(statuses);
            tab->refreshTable();
        }
    }
}

/**
 * @brief Dynamically loads and populates all repository tabs in the tab widget.
 */
void Klass::loadRepositoryTabs() {
    tabWidget->clear();
    pendingInstalls.clear();
    pendingUpdates.clear();
    pendingReinstalls.clear();
    pendingRemoves.clear();

    QList<RuleEntry> currentRules = RulesManager::loadRules();
    QList<PkgInfo> allInstalled = packagesManager->getInstalledPackages();
    QMap<QString, RepoData> repoMap = packagesManager->getAvailablePackages();

    QMap<QString, QString> mMap;
    int totalRepoPackages = 0;
    QHash<QString, QSet<QString> > repoTagsCache;
    repoTagsCache.reserve(repoMap.size());

    for (auto it = repoMap.begin(); it != repoMap.end(); ++it) {
        totalRepoPackages += static_cast<int>(it.value().packages.size());
        mMap.insert(it.key(), it.value().mirrorUrl);

        QSet<QString> &tags = repoTagsCache[it.key()];
        for (const auto &pkg: it.value().packages) {
            const int buildPos = static_cast<int>(pkg.version.lastIndexOf(u'-'));
            const QString build = buildPos >= 0 ? pkg.version.mid(buildPos + 1) : pkg.version;

            int tagPos = 0;
            while (tagPos < build.length() && build.at(tagPos).isDigit())
                ++tagPos;
            tags.insert(tagPos < build.length() ? build.mid(tagPos) : QString{});
        }
    }

    QList<PkgInfo> allPkgs;
    allPkgs.reserve(totalRepoPackages + allInstalled.size());
    QSet<std::pair<QString, QString> > existingPackages;
    existingPackages.reserve(totalRepoPackages);

    for (auto it = repoMap.begin(); it != repoMap.end(); ++it) {
        const auto &repoPackages = it.value().packages;
        allPkgs.append(repoPackages);
        for (const auto &pkg: repoPackages)
            existingPackages.insert({pkg.name, pkg.version});
    }

    QStringList sortedRepos;
    sortedRepos.reserve(repoMap.size());
    for (auto it = repoMap.keyBegin(); it != repoMap.keyEnd(); ++it)
        sortedRepos.append(*it);

    sortedRepos.removeOne(SLACK_OFICIAL);
    sortedRepos.removeOne(SLACK_PATCHES);
    sortedRepos.removeOne(SLACK_TESTING);
    sortedRepos.removeOne(SLACK_EXTRA);
    sortedRepos.sort();
    sortedRepos.prepend(SLACK_EXTRA);
    sortedRepos.prepend(SLACK_TESTING);
    sortedRepos.prepend(SLACK_PATCHES);
    sortedRepos.prepend(SLACK_OFICIAL);

    if (rulesManagerDialog) {
        rulesManagerDialog->setRepositories(sortedRepos);
        rulesManagerDialog->setPriorityRepositories(sortedRepos);
    }

    bool attachTestingEnabled = false;
    QStringList mergedIntoSlackware;

    if (settingsManager) {
        attachTestingEnabled = settingsManager->attachTesting();
        if (settingsManager->attachPatches()) {
            for (const QString &repoName: sortedRepos) {
                if (repoName.contains(SLACK_PATCHES))
                    mergedIntoSlackware.append(repoName);
            }
        }
        if (attachTestingEnabled) {
            for (const QString &repoName: sortedRepos) {
                if (repoName.contains(SLACK_TESTING))
                    mergedIntoSlackware.append(repoName);
            }
        }
    }

    QMap<QString, QList<PkgInfo> > extraRepoPackages;
    QList<PkgInfo> orphanPackages;
    orphanPackages.reserve(allInstalled.size());

    for (const auto &inst: allInstalled) {
        if (existingPackages.contains({inst.name, inst.version}))
            continue;

        QString resolvedRepo;
        const int instBuildPos = static_cast<int>(inst.version.lastIndexOf(u'-'));
        const QString instBuild = instBuildPos >= 0 ? inst.version.mid(instBuildPos + 1) : inst.version;

        int installedTagPos = 0;
        while (installedTagPos < instBuild.length() && instBuild.at(installedTagPos).isDigit())
            ++installedTagPos;
        const QString installedTag = installedTagPos < instBuild.length() ? instBuild.mid(installedTagPos) : QString{};

        for (auto it = repoMap.constBegin(); it != repoMap.constEnd(); ++it) {
            const QString &rName = it.key();
            const auto &availList = it.value().packages;

            auto match = std::ranges::find_if(availList, [&](const PkgInfo &p) {
                return p.name == inst.name;
            });

            if (match != availList.end()) {
                if (!installedTag.isEmpty()) {
                    if (repoTagsCache.value(rName).contains(installedTag)) {
                        resolvedRepo = rName;
                        break;
                    }
                } else {
                    if (rName == SLACK_OFICIAL || rName == SLACK_PATCHES || rName == SLACK_TESTING || rName ==
                        SLACK_EXTRA) {
                        resolvedRepo = rName;
                        break;
                    }
                }
            }
        }

        if (!resolvedRepo.isEmpty()) {
            PkgInfo matchedPkg = inst;
            matchedPkg.repoName = resolvedRepo;
            extraRepoPackages[resolvedRepo].append(matchedPkg);
        } else {
            PkgInfo orphanPkg = inst;
            orphanPkg.repoName = SLACK_OTHERS;
            orphanPackages.append(orphanPkg);
        }
    }

    for (auto it = extraRepoPackages.begin(); it != extraRepoPackages.end(); ++it)
        allPkgs.append(it.value());
    allPkgs.append(orphanPackages);

    const QHash<PkgKey, RuleSt> ruleStatuses = RulesManager::resolveStatuses(allPkgs, currentRules);

    auto *allTab = new RepositoryTab(tabWidget, false, packagesManager);
    allTab->setProperty("repoName", ALL_REPOSITORIES);
    allTab->setProperty("attachTesting", attachTestingEnabled);
    allTab->setMirrorMap(mMap);
    allTab->setRuleStatuses(ruleStatuses);
    allTab->setGlobalPackages(allPkgs);
    allTab->fillTable(allPkgs);
    connectTabSignals(allTab);
    tabWidget->addTab(allTab, tr("All Packages"));

    for (const QString &repoName: sortedRepos) {
        if (mergedIntoSlackware.contains(repoName))
            continue;

        if (auto it = repoMap.find(repoName); it != repoMap.end()) {
            const auto &repo = it.value();

            QList<PkgInfo> combinedPackages = repo.packages;
            if (extraRepoPackages.contains(repoName))
                combinedPackages.append(extraRepoPackages.value(repoName));

            if (repoName == SLACK_OFICIAL) {
                for (const QString &mergedRepo: mergedIntoSlackware) {
                    if (auto mIt = repoMap.find(mergedRepo); mIt != repoMap.end())
                        combinedPackages.append(mIt.value().packages);
                    if (extraRepoPackages.contains(mergedRepo))
                        combinedPackages.append(extraRepoPackages.value(mergedRepo));
                }
            }

            auto *repoTab = new RepositoryTab(tabWidget, repo.hasCategories, packagesManager);
            repoTab->setProperty("repoName", repoName);
            repoTab->setProperty("attachTesting", attachTestingEnabled);
            repoTab->setMirrorMap(mMap);
            repoTab->setRuleStatuses(ruleStatuses);
            repoTab->setGlobalPackages(allPkgs);
            repoTab->fillTable(combinedPackages);
            connectTabSignals(repoTab);

            tabWidget->addTab(repoTab, repoName);
        }
    }

    auto *uncategorizedTab = new RepositoryTab(tabWidget, false, packagesManager);
    uncategorizedTab->setProperty("repoName", SLACK_OTHERS);
    uncategorizedTab->setProperty("attachTesting", attachTestingEnabled);
    uncategorizedTab->setMirrorMap(mMap);
    uncategorizedTab->setRuleStatuses(ruleStatuses);
    uncategorizedTab->setGlobalPackages(allPkgs);
    uncategorizedTab->fillTable(orphanPackages);
    connectTabSignals(uncategorizedTab);
    tabWidget->addTab(uncategorizedTab, tr("Others"));

    allPackages = allPkgs;
    mirrorMap = mMap;

    if (const QString query = searchBar->text().trimmed(); !query.isEmpty()) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (auto *tab = qobject_cast<RepositoryTab *>(tabWidget->widget(i)))
                tab->setSearchFilter(query);
        }
    }
}

/**
 * @brief Reacts to changes in the search field, debouncing the per-tab filtering.
 * @param text The current content of the search bar.
 */
void Klass::onSearchTextChanged(const QString &text) const {
    if (text.trimmed().isEmpty()) {
        searchDebounceTimer->stop();
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (const auto *tab = qobject_cast<RepositoryTab *>(tabWidget->widget(i)))
                tab->setSearchFilter(QString{});
        }
    } else {
        searchDebounceTimer->start();
    }
}

/**
 * @brief Handles the completion of the repository update process.
 */
void Klass::onUpdateFinished() {
    mirrorManager->pendingUpdate(false);
    updateNotificationShown = false;
    packagesManager->reload();
    loadRepositoryTabs();
}

/**
 * @brief Handles transaction completion.
 */
void Klass::onTransactionFinished() {
    packagesManager->reload();
    loadRepositoryTabs();
}

/**
 * @brief Handles a request to change the active repository mirror.
 * @param url The URL of the new official mirror to be applied.
 */
void Klass::handleMirrorChangeRequested(const QString &url) {
    pendingMirrorUrl = url;
    helper->setupHelper(PendingAction::SetMirror);
}

/**
 * @brief Handles a request to enable a set of third-party repositories.
 * @param entries List of third-party mirror configurations to be enabled.
 */
void Klass::handleReposEnable(const QList<Mirrors::MirrorPlusEntry> &entries) {
    pendingRepoEnable = entries;
    helper->setupHelper(PendingAction::EnableRepo);
}

/**
 * @brief Handles a request to remove or disable third-party repositories.
 * @param names List of third-party repository names to be disabled.
 */
void Klass::handleReposRemoved(const QStringList &names) {
    pendingRepoDisable = names;
    helper->setupHelper(PendingAction::DisableRepo);
}

/**
 * @brief Slot triggered when the update action is requested.
 */
void Klass::onUpdate() {
    if (const auto [url, country] = Mirrors::activeOne(); url.isEmpty()) {
        QMessageBox::warning(this, tr("Configuration Required"), tr("Enable official mirror to update."));
        return;
    }

    if (!updateNotificationShown && !mirrorManager->pendingUpdate()) {
        const QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Update Database"),
            tr("No update notification was received. Would you like to update the database anyway?"),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::No)
            return;
    }

    helper->setupHelper(PendingAction::Update);
}

/**
 * @brief Handles the request to add a new exception or priority rule.
 * @param addRule The RuleEntry object containing the rule details to be added.
 */
void Klass::handleAddRuleRequested(const RuleEntry &addRule) {
    rule = addRule;
    helper->setupHelper(PendingAction::AddRule);
}

/**
 * @brief Handles the request to delete an existing exception or priority rule.
 * @param rules The RuleEntry list containing the rule details to be deleted.
 */
void Klass::handleDeleteRulesRequested(const QList<RuleEntry> &rules) {
    pendingRules = rules;
    helper->setupHelper(PendingAction::DeleteRule);
}

/**
 * @brief Handles the request to edit an existing exception or priority rule.
 * @param oldRule The rule as it currently exists (used to locate it for removal).
 * @param newRule The rule's new values (used to add the replacement).
 */
void Klass::handleEditRuleRequested(const RuleEntry &oldRule, const RuleEntry &newRule) {
    rule = oldRule;
    edit = newRule;
    helper->setupHelper(PendingAction::EditRule);
}

/**
 * @brief Processes a single package by its identifier.
 * @param file The name or path of the package to be processed.
 */
void Klass::handlePackage(const QString &file) const {
    installDialog->setPackage(file);
    installDialog->show();
}

/**
 * @brief Handles helper readiness and dispatches the pending action.
 */
void Klass::onHelperReady() {
    debug->msg("Helper is ready", "Klass");

    if (const PendingAction pending = helper->pendingAct(); pending == PendingAction::Update) {
        terminalDialog->setWindowTitle(tr("Update Slackware Database"));
        terminalDialog->setOperationRequest(true);
        terminalDialog->show();
        helper->socket()->write(QByteArray("UPDATE") + SEP);
    } else if (pending == PendingAction::SetMirror && !pendingMirrorUrl.isEmpty()) {
        helper->socket()->write(QByteArray("SETMIRROR:") + pendingMirrorUrl.toUtf8() + SEP);
        mirrorManager->officialMirrorWidget()->setText(pendingMirrorUrl);
        mirrorManager->pendingUpdate(true);
        pendingMirrorUrl.clear();
    } else if (pending == PendingAction::EnableRepo && !pendingRepoEnable.isEmpty()) {
        QStringList parts;
        for (const auto &[name, url]: pendingRepoEnable)
            parts << name + "=" + url;
        helper->socket()->write(QByteArray("ENABLEREPOS:") + parts.join(';').toUtf8() + SEP);
        mirrorManager->thirdMirrorUpdate(pendingRepoEnable);
        mirrorManager->pendingUpdate(true);
        pendingRepoEnable.clear();
    } else if (pending == PendingAction::DisableRepo && !pendingRepoDisable.isEmpty()) {
        helper->socket()->write(QByteArray("DISABLEREPOS:") + pendingRepoDisable.join(';').toUtf8() + SEP);
        mirrorManager->thirdMirrorUpdate(pendingRepoDisable);
        mirrorManager->pendingUpdate(true);
        pendingRepoDisable.clear();
    } else if (pending == PendingAction::AddRule) {
        // Um de cada vez, não precisa ser em massa
        const QString p = QString("ADDRULE:%1,%2,%3,%4").arg(rule.type, rule.repo, rule.scope, rule.rule);
        helper->socket()->write(p.toUtf8() + SEP);
        rulesManagerDialog->appendRuleToTable(rule);
    } else if (pending == PendingAction::DeleteRule) {
        QStringList ruleStrings{};
        for (const auto &r: pendingRules)
            ruleStrings.append(QString("%1,%2,%3,%4").arg(r.type, r.repo, r.scope, r.rule));
        const QString p = QString("DELRULES:%1").arg(ruleStrings.join(";")); // Deletar em massa
        helper->socket()->write(p.toUtf8() + SEP);
        rulesManagerDialog->removeRulesFromTable(pendingRules);
    } else if (pending == PendingAction::EditRule) {
        // Um de cada vez, não precisa ser em massa
        const QString oldPart = QString("%1,%2,%3,%4").arg(rule.type, rule.repo, rule.scope, rule.rule);
        const QString newPart = QString("%1,%2,%3,%4").arg(edit.type, edit.repo, edit.scope, edit.rule);
        helper->socket()->write(QByteArray("EDITRULE:") + oldPart.toUtf8() + "|" + newPart.toUtf8() + SEP);
        rulesManagerDialog->replaceRuleInTable(rule, edit);
    } else if (pending == PendingAction::ProcessTransaction) {
        terminalDialog->setWindowTitle(tr("Processing Packages"));
        terminalDialog->setOperationRequest(true);
        terminalDialog->show();

        auto formatPkgList = [](const QList<PendingPkg> &list) {
            QStringList formatted;
            for (const auto &p: list)
                // Formato: nome|versão|repo|categoria|url_pacote|url_asc|md5_pacote|md5_asc
                formatted << QString("%1|%2|%3|%4|%5|%6|%7|%8")
                        .arg(p.name, p.version, p.repoName, p.category, p.fullDownloadUrl, p.ascUrl, p.md5sum,
                             p.ascMd5sum);
            return formatted.join(',');
        };

        QString payload = "TRANSACTION:";
        if (!pendingInstalls.isEmpty()) payload += "INSTALL=" + formatPkgList(pendingInstalls) + ";";
        if (!pendingUpdates.isEmpty()) payload += "UPDATE=" + formatPkgList(pendingUpdates) + ";";
        if (!pendingReinstalls.isEmpty()) payload += "REINSTALL=" + formatPkgList(pendingReinstalls) + ";";
        if (!pendingRemoves.isEmpty()) payload += "REMOVE=" + formatPkgList(pendingRemoves) + ";";

        helper->socket()->write(payload.toUtf8() + SEP);
    }
    helper->setPendingAct(PendingAction::None);
}

/**
 * @brief Displays a desktop notification when a package database update is available.
 */
void Klass::onUpdateAvailable() {
    if (updateNotificationShown || !tray) return;

    updateNotificationShown = true;
    const auto title = tr("Database Update Available").toStdString();
    const auto text = tr("A newer Slackware package database is available.").toStdString();
    notify_send(title.c_str(), text.c_str(), pathIcon.c_str());
}

/**
 * @brief Intercepts the close event to manage system tray minimization instead of closing.
 * @param event The QCloseEvent instance.
 */
void Klass::closeEvent(QCloseEvent *event) {
    if (!this->isMaximized()) // Se tiver maximizado dá zica
        settingsManager->windowGeometry(this->geometry());

    if (tray->getTray()->isVisible()) {
        if (helper->socket()->state() == QLocalSocket::ConnectedState) {
            helper->setHelperShutdownRequested();
            helper->socket()->write(QByteArray("IDLE") + SEP);
        }
        this->hide();
        event->ignore();
    } else {
        event->accept();
    }
}

/**
 * @brief Monitors window state changes for persistence management.
 * @param event The QEvent instance representing the change.
 */
void Klass::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        switch (this->windowState()) {
            case Qt::WindowNoState:
                debug->msg("Restoring program window", "Klass");
                settingsManager->windowMaximize(false);
                break;
            case Qt::WindowMaximized:
                debug->msg("Maximizing program window", "Klass");
                settingsManager->windowGeometry(this->geometry()); // Tem que salvar antes
                settingsManager->windowMaximize(true);
                break;
            default: ;
        }
    }
    QWidget::changeEvent(event);
}

/**
 * @brief Handles window show event and notifies helper.
 * @param event The show event instance.
 */
void Klass::showEvent(QShowEvent *event) {
    if (!started) {
        QTimer::singleShot(1, this, [this] { loadRepositoryTabs(); }); // Respiro para iniciar a interface primeiro
        started = true;
    }

    if (helper->socket()->state() == QLocalSocket::ConnectedState)
        helper->socket()->write(QByteArray("ACTIVE") + SEP);

    helper->onHelperConnected();
    QWidget::showEvent(event);
}
