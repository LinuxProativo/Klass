/**
 * @file SummaryDialog.cpp
 * @brief Implementation of the TransactionSummaryDialog interface for reviewing package changes.
 */

#include <QHash>
#include <QTreeWidgetItem>

#include <SummaryDialog.hpp>

namespace {
    /**
     * @brief Formats a PendingPackage into a human-readable display string.
     * @param pkg The pending package to format.
     * @return Formatted label string.
     */
    QString formatPackageLabel(const PendingPkg &pkg) {
        if (!pkg.previousVersion.isEmpty())
            return QStringLiteral("%1-%2 ⟶ %1-%3").arg(pkg.name, pkg.previousVersion, pkg.version);

        return QStringLiteral("%1-%2").arg(pkg.name, pkg.version);
    }

    /**
     * @brief Groups packages by repo name, preserving first-seen repo order, so each category
     *        branch lists its repos as sub-branches instead of repeating the repo per package.
     * @param pkgs The packages to group.
     * @return Ordered list of (repoName, packages) groups.
     */
    QList<QPair<QString, QList<PendingPkg> > > groupByRepo(const QList<PendingPkg> &pkgs) {
        QList<QPair<QString, QList<PendingPkg> > > groups;
        QHash<QString, int> indexByRepo;

        for (const auto &pkg: pkgs) {
            const QString repo = pkg.repoName.isEmpty() ? QObject::tr("Unknown") : pkg.repoName;

            if (const auto it = indexByRepo.constFind(repo); it != indexByRepo.constEnd()) {
                groups[it.value()].second.append(pkg);
            } else {
                indexByRepo.insert(repo, static_cast<int>(groups.size()));
                groups.append({repo, {pkg}});
            }
        }

        return groups;
    }
}

/**
 * @brief Sets up window modality, size behaviors, and builds the layout displaying categorized package transactions.
 * @param parent Pointer to the parent widget container.
 */
SummaryDialog::SummaryDialog(QWidget *parent) : Dialog(parent, Qt::ApplicationModal) {
    this->setWindowTitle(tr("Transaction Summary"));
    this->minimum();
    this->fixed();

    headerLabel = new QLabel(tr("The following package changes are ready to be processed:"), this);
    headerLabel->setWordWrap(true);

    summaryTree = new TreeWidget(this);
    summaryTree->setColumnCount(1);
    summaryTree->setHeaderHidden(true);
    summaryTree->setAnimated(true);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(headerLabel);
    mainLayout->addWidget(summaryTree);

    totalStatsLabel = new QLabel(tr("Summary: 0 operations pending."), this);
    totalStatsLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    mainLayout->addWidget(totalStatsLabel);

    cancelBtn = new QPushButton(tr("Cancel"), this);
    confirmBtn = new QPushButton(tr("Proceed"), this);
    confirmBtn->setDefault(true);

    buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(confirmBtn);
    mainLayout->addLayout(buttonLayout);

    connect(confirmBtn, &QPushButton::clicked, this, &SummaryDialog::onConfirmClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &SummaryDialog::onCancelClicked);
}

/**
 * @brief Rebuilds the summary tree from the current pending queues and updates the operation count.
 * @param installs Packages queued for a fresh installation.
 * @param updates Packages queued for an upgrade.
 * @param reinstalls Packages queued for a reinstall.
 * @param removes Packages queued for removal.
 */
void SummaryDialog::loadTransaction(const QList<PendingPkg> &installs, const QList<PendingPkg> &updates,
                                    const QList<PendingPkg> &reinstalls, const QList<PendingPkg> &removes) {
    summaryTree->clear();

    const int installCount = addInstallGroup(summaryTree, tr("To Install"), installs);
    const int updateCount = addGroup(upgradeGroup, summaryTree, tr("To Upgrade"), updates);
    const int reinstallCount = addGroup(reinstallGroup, summaryTree, tr("To Reinstall"), reinstalls);
    const int removeCount = addGroup(removeGroup, summaryTree, tr("To Remove"), removes);

    summaryTree->expandAll();

    const int total = installCount + updateCount + reinstallCount + removeCount;
    totalStatsLabel->setText(tr("Summary: %1 operation(s) pending.").arg(total));
    confirmBtn->setEnabled(total > 0);
}

/**
 * @brief Populates a tree widget with installation packages, resolving dependency hierarchies.
 * @param tree Target tree widget container.
 * @param title Group header title.
 * @param pkgs List of installation packages with potential dependency attributes.
 * @return Total count of added packages.
 */
int SummaryDialog::addInstallGroup(QTreeWidget *tree, const QString &title, const QList<PendingPkg> &pkgs) {
    if (pkgs.isEmpty())
        return 0;

    installGroup = new QTreeWidgetItem(tree, {title});

    for (const auto &[repoName, repoPkgs]: groupByRepo(pkgs)) {
        auto *repoBranch = new QTreeWidgetItem(installGroup, {repoName}); // NOLINT

        QHash<QString, QTreeWidgetItem *> itemsByName;
        QList<PendingPkg> remaining;

        for (const auto &pkg: repoPkgs) {
            if (pkg.dependencyOf.isEmpty()) {
                itemsByName.insert(pkg.name, new QTreeWidgetItem(repoBranch, {formatPackageLabel(pkg)}));
            } else {
                remaining.append(pkg);
            }
        }

        bool progress = true;
        while (!remaining.isEmpty() && progress) {
            progress = false;
            for (int i = static_cast<int>(remaining.size()) - 1; i >= 0; --i) {
                const PendingPkg &pkg = remaining.at(i);
                if (auto *parentItem = itemsByName.value(pkg.dependencyOf, nullptr)) {
                    itemsByName.insert(pkg.name, new QTreeWidgetItem(parentItem, {formatPackageLabel(pkg)}));
                    remaining.removeAt(i);
                    progress = true;
                }
            }
        }

        for (const auto &pkg: remaining)
            new QTreeWidgetItem(repoBranch, {formatPackageLabel(pkg) + QStringLiteral(" — ") + tr("dependency")});
    }

    return static_cast<int>(pkgs.size()); // NOLINT
}

/**
 * @brief Populates a tree widget with a package group, branching first by repo, then listing
 *        each repo's own packages underneath.
 * @param g Reference to the class member pointer that will hold the root group item.
 * @param tree Target tree widget container.
 * @param t Group header title.
 * @param p List of packages for this category.
 * @return Total count of added packages.
 */
int SummaryDialog::addGroup(QTreeWidgetItem *&g, QTreeWidget *tree, const QString &t, const QList<PendingPkg> &p) {
    if (p.isEmpty()) {
        g = nullptr;
        return 0;
    }

    g = new QTreeWidgetItem(tree, {t});

    for (const auto &[repoName, repoPkgs]: groupByRepo(p)) {
        auto *repoBranch = new QTreeWidgetItem(g, {repoName}); // NOLINT
        for (const auto &pkg: repoPkgs)
            new QTreeWidgetItem(repoBranch, {formatPackageLabel(pkg)});
    }

    return static_cast<int>(p.size()); // NOLINT
}
