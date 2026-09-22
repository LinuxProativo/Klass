/**
 * @file SummaryDialog.hpp
 * @brief Header for summary of packages actions.
 */

#ifndef SUMMARYDIALOG_HPP
#define SUMMARYDIALOG_HPP

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <Dialog.hpp>
#include <RepositoryTab.hpp>
#include <TreeWidget.hpp>

/**
 * @class SummaryDialog
 * @brief Dialog window that displays a structured staging summary of packages actions.
 */
class SummaryDialog : public Dialog {
    Q_OBJECT

public:
    explicit SummaryDialog(QWidget *parent = nullptr);

    void loadTransaction(const QList<PendingPkg> &installs, const QList<PendingPkg> &updates,
                         const QList<PendingPkg> &reinstalls, const QList<PendingPkg> &removes);

private:
    int addInstallGroup(QTreeWidget *tree, const QString &title, const QList<PendingPkg> &pkgs);

    static int addGroup(QTreeWidgetItem *&g, QTreeWidget *tree, const QString &t, const QList<PendingPkg> &p);

    void onConfirmClicked() { this->accept(); }

    void onCancelClicked() { this->reject(); }

    QHBoxLayout *buttonLayout{};
    QLabel *totalStatsLabel{}, *headerLabel{};
    QPushButton *confirmBtn{}, *cancelBtn{};
    QTreeWidgetItem *installGroup{}, *upgradeGroup{}, *reinstallGroup{}, *removeGroup{};
    QVBoxLayout *mainLayout{};

    TreeWidget *summaryTree{};
};

#endif
