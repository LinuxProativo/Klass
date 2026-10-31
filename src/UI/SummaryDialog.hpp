/**
* @file SummaryDialog.hpp
 * @brief Header for summary of packages actions using TreeWidget (QTreeView based).
 */

#ifndef SUMMARYDIALOG_HPP
#define SUMMARYDIALOG_HPP

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
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
    explicit SummaryDialog(QWidget *parent = {});

    ~SummaryDialog() override;

    void loadTransaction(const QList<PendingPkg> &installs, const QList<PendingPkg> &updates,
                         const QList<PendingPkg> &reinstalls, const QList<PendingPkg> &removes) const;

private:
    int addInstallGroup(const QString &title, const QList<PendingPkg> &pkgs) const;

    int addGroup(const QString &title, const QList<PendingPkg> &pkgs) const;

    void onConfirmClicked() { this->accept(); }

    void onCancelClicked() { this->reject(); }

    QHBoxLayout *buttonLayout{};
    QLabel *totalStatsLabel{}, *headerLabel{};
    QPushButton *confirmBtn{}, *cancelBtn{};
    QVBoxLayout *mainLayout{};

    TreeWidget *summaryTree{};
    QStandardItemModel *summaryModel{};
};

#endif
