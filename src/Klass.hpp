/**
 * @file Klass.hpp
 * @brief Header for main interface.
 */

#ifndef KLASS_HPP
#define KLASS_HPP

#include <QHash>
#include <QPointer>
#include <QTabWidget>
#include <QTimer>

#include <Buttons.hpp>
#include <HelperController.hpp>
#include <MirrorManager.hpp>
#include <Mirrors.hpp>
#include <Packages.hpp>
#include <RepoVerificationManager.hpp>
#include <RepositoryTab.hpp>
#include <RulesManagerDialog.hpp>
#include <SettingsManager.hpp>
#include <StatusBar.hpp>
#include <Terminal.hpp>

/**
 * @class Klass
 * @brief Main application window managing the layout and interface components.
 */
class Klass final : public QWidget {
    Q_OBJECT

public:
    explicit Klass(QWidget *parent = nullptr);

    void handlePackage(const QString &file) const;

    void onUpdateAvailable();

    void setCheckingStatus(bool checking) const;

signals:
    void databaseUpdateFinished();

protected:
    void closeEvent(QCloseEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void changeEvent(QEvent *event) override;

private:
    void applySearchFilterToAllTabs() const;

    void handlePackageAction(const QList<PendingPkg> &pkgs, ActionType type);

    void handlePackageUnselectBatch(const QList<PendingPkg> &pkgs);

    void notifyAllTabsBatch(const QList<PendingPkg> &pkgs, PkgStatus status) const;

    void reloadRules() const;

    void loadRepositoryTabs();

    void connectTabSignals(const RepositoryTab *tab) const;

    void onUpdateFinished();

    void onSelectUpdates();

    void onTransactionFinished();

    void onApplyTransaction();

    void onSearchTextChanged(const QString &text) const;

    void onOpenSettings();

    void onOpenMirrorManager();

    void onOpenRulesManager();

    void onShowAbout();

    void onShowChangelog();

    void handleMirrorChangeRequested(const QString &url);

    void handleReposEnable(const QList<Mirrors::MirrorPlusEntry> &entries);

    void handleReposRemoved(const QStringList &names);

    void handleOfficialBranchesChanged(const Mirrors::OfficialBranches &branches);

    void handleRepoVerificationChanged(const RepoVerificationEntry &entry);

    void onMirrorManagerClosed();

    void handleAddRuleRequested(const RuleEntry &addRule);

    void handleDeleteRulesRequested(const QList<RuleEntry> &rules);

    void handleEditRuleRequested(const RuleEntry &oldRule, const RuleEntry &newRule);

    void handleAdminConfigSaveRequested(const QString &payload);

    void onUpdate();

    void onHelperReady();

    void updateStatusIndicator() const;

    QHBoxLayout *searchLayout{}, *toolBarLayout{};
    QLineEdit *searchBar{};
    QTabWidget *tabWidget{};
    QVBoxLayout *mainLayout{}, *tabLayout{};
    QWidget *toolBarWidget{};
    QTimer *searchDebounceTimer{};

    Buttons *btnAdd{}, *btnException{}, *btnUpdate{}, *btnUp{}, *btnApply{};
    Buttons *btnSettings{}, *btnChangelog{}, *btnAbout{};
    HelperController *helper{};
    Packages *packagesManager{};
    SettingsManager *settingsManager{};
    StatusBar *statusBar{};
    Terminal *terminalDialog{};

    QPointer<MirrorManager> mirrorManager{nullptr};
    QPointer<RulesManagerDialog> rulesManagerDialog{nullptr};
    RuleEntry rule{}, edit{};

    QHash<QString, QList<const PkgInfo *> > globalByName;
    QList<PendingPkg> pendingInstalls{}, pendingReinstalls{}, pendingRemoves{}, pendingUpdates{};
    QList<PkgInfo> orphanAndExtraPackages{};
    QList<const PkgInfo *> allPackages{};
    QList<Mirrors::MirrorPlusEntry> pendingRepoEnable{};
    QList<RuleEntry> pendingRules{};
    QMap<QString, QString> mirrorMap{};
    QStringList pendingRepoDisable{};
    QStringList cachedRepos{};
    QString pendingMirrorUrl{}, pendingAdminConfigPayload{};
    Mirrors::OfficialBranches pendingOfficialBranches{};
    RepoVerificationEntry pendingRepoVerification{};
    bool updateNotificationShown{false}, started{false}, hasPendingMirrorUpdate{false};
};

#endif
