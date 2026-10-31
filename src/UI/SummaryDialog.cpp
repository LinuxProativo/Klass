/**
 * @file SummaryDialog.cpp
 * @brief Implementation of SummaryDialog using TreeWidget with QStandardItemModel.
 */

#include <QHash>
#include <QHeaderView>

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
     * @brief Groups packages by repo name, preserving first-seen repo order.
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
    this->setAttribute(Qt::WA_DeleteOnClose, false);
    this->minimum();
    this->fixed();

    headerLabel = new QLabel(tr("The following package changes are ready to be processed:"), this);
    headerLabel->setWordWrap(true);

    summaryModel = new QStandardItemModel(this);
    summaryTree = new TreeWidget(this);
    summaryTree->setModel(summaryModel);
    summaryTree->setHeaderHidden(true);
    summaryTree->setAnimated(true);
    summaryTree->setEditTriggers(QAbstractItemView::NoEditTriggers);

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
 * @brief Destructor releasing summary model items.
 */
SummaryDialog::~SummaryDialog() {
    if (summaryModel)
        summaryModel->clear();
}

/**
 * @brief Rebuilds the summary tree from the current pending queues.
 */
void SummaryDialog::loadTransaction(const QList<PendingPkg> &installs, const QList<PendingPkg> &updates,
                                    const QList<PendingPkg> &reinstalls, const QList<PendingPkg> &removes) const {
    summaryModel->clear();

    const int installCount = addInstallGroup(tr("To Install"), installs);
    const int updateCount = addGroup(tr("To Upgrade"), updates);
    const int reinstallCount = addGroup(tr("To Reinstall"), reinstalls);
    const int removeCount = addGroup(tr("To Remove"), removes);

    summaryTree->expandAll();

    const int total = installCount + updateCount + reinstallCount + removeCount;
    totalStatsLabel->setText(tr("Summary: %1 operation(s) pending.").arg(total));
    confirmBtn->setEnabled(total > 0);
}

/**
 * @brief Populates the tree model with installation packages, resolving dependency hierarchies.
 */
int SummaryDialog::addInstallGroup(const QString &title, const QList<PendingPkg> &pkgs) const {
    if (pkgs.isEmpty())
        return 0;

    auto *groupItem = new QStandardItem(title); // NOLINT
    summaryModel->appendRow(groupItem);

    for (const auto &[repoName, repoPkgs]: groupByRepo(pkgs)) {
        auto *repoBranch = new QStandardItem(repoName); // NOLINT
        groupItem->appendRow(repoBranch);

        QHash<QString, QStandardItem *> itemsByName;
        QList<PendingPkg> remaining;

        for (const auto &pkg: repoPkgs) {
            if (pkg.dependencyOf.isEmpty()) {
                auto *pkgItem = new QStandardItem(formatPackageLabel(pkg)); // NOLINT
                repoBranch->appendRow(pkgItem);
                itemsByName.insert(pkg.name, pkgItem);
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
                    auto *depItem = new QStandardItem(formatPackageLabel(pkg)); // NOLINT
                    parentItem->appendRow(depItem);
                    itemsByName.insert(pkg.name, depItem);
                    remaining.removeAt(i);
                    progress = true;
                }
            }
        }

        for (const auto &pkg: remaining) {
            repoBranch->appendRow(new QStandardItem(formatPackageLabel(pkg) + QStringLiteral(" — ") + tr("dependency")));
        }
    }

    return static_cast<int>(pkgs.size()); // NOLINT
}

/**
 * @brief Populates the tree model with a categorized package group.
 */
int SummaryDialog::addGroup(const QString &title, const QList<PendingPkg> &pkgs) const {
    if (pkgs.isEmpty())
        return 0;

    auto *groupItem = new QStandardItem(title); // NOLINT
    summaryModel->appendRow(groupItem);

    for (const auto &[repoName, repoPkgs]: groupByRepo(pkgs)) {
        auto *repoBranch = new QStandardItem(repoName); // NOLINT
        groupItem->appendRow(repoBranch);

        for (const auto &pkg: repoPkgs)
            repoBranch->appendRow(new QStandardItem(formatPackageLabel(pkg)));
    }

    return static_cast<int>(pkgs.size()); // NOLINT
}
