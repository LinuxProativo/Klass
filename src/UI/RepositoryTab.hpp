/**
 * @file RepositoryTab.hpp
 * @brief Header for view packages using RepositoryTab.
 */

#ifndef REPOSITORYTAB_HPP
#define REPOSITORYTAB_HPP

#include <QAction>
#include <QHash>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>

#include <CategoryFilterProxyModel.hpp>
#include <Debug.hpp>
#include <Packages.hpp>
#include <RulesManager.hpp>
#include <SlackwareDefines.hpp>
#include <StatusIcons.hpp>
#include <TableView.hpp>
#include <TreeWidget.hpp>
#include <VersionComboBoxDelegate.hpp>

/**
 * @enum ActionType
 * @brief Identifies package actions requested by shortcuts, context menus, or operations.
 */
enum class ActionType {
    Install,
    Update,
    Reinstall,
    Remove,
    Unselect
};

/**
 * @struct PendingPkg
 * @brief Represents a package queued for installation or download.
 */
struct PendingPkg {
    QString name{};
    QString version{};
    QString repoName{};
    QString category{};
    QString fullDownloadUrl{};
    QString ascUrl{};
    QString md5sum{};
    QString ascMd5sum{};
    QString dependencyOf{};
    QString previousVersion{};
    bool hasAsc{false};
};

/**
 * @class RepositoryTab
 * @brief Manages the display of package repositories, providing a categorized view and package inspection interface.
 */
class RepositoryTab : public QWidget {
    Q_OBJECT

public:
    explicit RepositoryTab(QWidget *parent, bool isvisible, Packages *packagesManager);

    void fillTable(const QList<PkgInfo> &pkgs);

    void setMirrorMap(const QMap<QString, QString> &mMap) { mirrorMap = mMap; }

    void setRuleStatuses(const QHash<PkgKey, RuleSt> &statuses) { ruleStatuses = statuses; }

    void updatePackageStatusBatch(const QList<PendingPkg> &pkgs, PkgStatus st) const;

    void refreshTable();

    void setGlobalPackages(const QList<PkgInfo> &all);

    [[nodiscard]] QList<PendingPkg> collectAvailableUpdates() const;

    [[nodiscard]] QList<PendingPkg> collectNewInstalls() const;

    void setSearchFilter(const QString &term) const { proxyModel->setSearchFilter(term); }

    [[nodiscard]] bool hasActiveSearch() const { return !proxyModel->searchFilter().isEmpty(); }

signals:
    void packageActionRequested(const QList<PendingPkg> &pkgs, ActionType type);

    void packageUnselectRequested(const QList<PendingPkg> &pkgs);

private:
    void executeCommand(ActionType cmd);

    void onInstallShortcut() { executeCommand(ActionType::Install); }

    void onUpgradeShortcut() { executeCommand(ActionType::Update); }

    void onReinstallShortcut() { executeCommand(ActionType::Reinstall); }

    void onRemoveShortcut() { executeCommand(ActionType::Remove); }

    void onUnselectShortcut() { executeCommand(ActionType::Unselect); }

    void onVersionChanged(const QModelIndex &pIndex, int vIndex);

    void appendFileBatch();

    void showContextMenu(const QPoint &pos);

    void onCategoryItemClicked(const QTreeWidgetItem *item) const;

    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void appendField(QTextCursor cursor, const QString &label, const QString &value) const;

    void appendDependencyField(QTextCursor cursor, const QString &label, const QString &value) const;

    void rebuildPackageRowIndex();

    [[nodiscard]] int getRepoPriority(const QString &repo, bool rowIsTestingOnly) const;

    [[nodiscard]] QList<PkgInfo> resolveCand(const QString &pkg, const QList<PkgInfo> &fallback, bool testing) const;

    [[nodiscard]] PendingPkg createPendingPackage(const QString &pkg, const QString &ver, const QString &repo) const;

    [[nodiscard]] PendingPkg resolveBestUpdate(const QString &pkg, const QList<PkgInfo> &list, bool testing) const;

    [[nodiscard]] PkgInfo findDependencyCandidate(const QString &depName, const QString &parentRepoName) const;

    void addInstallWithDependencies(const QString &pkgName, const QString &version, const QString &repoName,
                                    QSet<QString> &addedToInstall, QList<PendingPkg> &installBatch,
                                    const QString &parentName = {}) const;

    QAction *installAction{}, *upgradeAction{}, *reinstallAction{}, *rollbackAction{};
    QAction *removeAction{}, *lockAction{}, *priorityAct{}, *unselectAct{};
    QSplitter *splitter{};
    QStandardItemModel *packageModel{};
    CategoryFilterProxyModel *proxyModel{};
    QTabWidget *detailsTab{};
    QTextEdit *infoText{};
    QTreeWidgetItem *root{};
    TreeWidget *fileList{};
    QVBoxLayout *layout{}, *repoLayout{};
    QWidget *detailsPanel{};

    Debug::Debug *debug{};
    Packages *packagesManager{};
    TableView *packageTable{};
    TreeWidget *catList{};
    VersionComboBoxDelegate *versionDelegate{};

    QList<PkgInfo> packages{};
    QHash<PkgKey, RuleSt> ruleStatuses{};
    QHash<QString, QList<PkgInfo> > globalByName{};
    QMap<QString, QString> mirrorMap{};
    QMultiHash<std::pair<QString, QString>, int> packageRowIndex{};
    QTextBlockFormat depMidFormat{}, depLastFormat{}, descFormat{}, fieldFormat{};
    QTextCharFormat boldFormat{}, normalFormat{};
    QStringList pendingFiles{};
    QString currentLoadSignature{};

    int pendingFileIndex{0}, pendingMaxWidth{0};
    bool showCategories{false};
};

#endif
