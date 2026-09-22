/**
 * @file Klass.hpp
 * @brief Header for main interface.
 */

#ifndef KLASS_HPP
#define KLASS_HPP

#include <QHash>
#include <QTabWidget>
#include <QTimer>

#include <About.hpp>
#include <Buttons.hpp>
#include <ChangeLog.hpp>
#include <HelperController.hpp>
#include <InstallPackage.hpp>
#include <MirrorManager.hpp>
#include <Mirrors.hpp>
#include <Packages.hpp>
#include <RepositoryTab.hpp>
#include <RulesManagerDialog.hpp>
#include <SettingsDialog.hpp>
#include <SettingsManager.hpp>
#include <SummaryDialog.hpp>
#include <SysTray.hpp>
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

    void onApplyTransaction() const;

    void onSearchTextChanged(const QString &text) const;

    void handleMirrorChangeRequested(const QString &url);

    void handleReposEnable(const QList<Mirrors::MirrorPlusEntry> &entries);

    void handleReposRemoved(const QStringList &names);

    void handleAddRuleRequested(const RuleEntry &addRule);

    void handleDeleteRulesRequested(const QList<RuleEntry> &rules);

    void handleEditRuleRequested(const RuleEntry &oldRule, const RuleEntry &newRule);

    void onUpdate();

    void onHelperReady();

    QHBoxLayout *searchLayout{}, *toolBarLayout{};
    QLineEdit *searchBar{};
    QTabWidget *tabWidget{};
    QVBoxLayout *mainLayout{};
    QWidget *toolBarWidget{};
    QTimer *searchDebounceTimer{};

    About *about{};
    Buttons *btnAdd{}, *btnException{};
    Buttons *btnUpdate{}, *btnUp{}, *btnApply{};
    Buttons *btnSettings{}, *btnChangelog{}, *btnAbout{};
    ChangeLog *logView{};
    Debug::Debug *debug{};
    InstallPackage::Install *installDialog{};
    HelperController *helper{};
    MirrorManager *mirrorManager{};
    Packages *packagesManager{};
    RulesManagerDialog *rulesManagerDialog{};
    SettingsDialog *settingsDialog{};
    SettingsManager *settingsManager{};
    SummaryDialog *summaryDialog{};
    SysTray *tray{};
    Terminal *terminalDialog{};

    std::string pathIcon{DefaultPath().defaultPath("icons/available.svg").toStdString()};
    RuleEntry rule{}, edit{};

    QList<PendingPkg> pendingInstalls{}, pendingReinstalls{}, pendingRemoves{}, pendingUpdates{};
    QList<PkgInfo> allPackages{};
    QList<Mirrors::MirrorPlusEntry> pendingRepoEnable{};
    QList<RuleEntry> pendingRules{};
    QMap<QString, QString> mirrorMap{};
    QStringList pendingRepoDisable{};
    QString pendingMirrorUrl{};
    bool updateNotificationShown{false}, started{false};
};

#endif
