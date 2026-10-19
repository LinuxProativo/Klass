/**
 * @file MirrorManager.hpp
 * @brief Header for configure mirrors using MirrorManager class.
 */

#ifndef MIRRORMANAGER_HPP
#define MIRRORMANAGER_HPP

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <Dialog.hpp>
#include <Mirrors.hpp>
#include <RepoVerificationManager.hpp>
#include <TableView.hpp>

/**
 * @class MirrorManager
 * @brief Provides a dialog for managing the official Slackware mirror and third-party repositories.
 */
class MirrorManager final : public Dialog {
    Q_OBJECT

public:
    explicit MirrorManager(QWidget *parent = nullptr);

    [[nodiscard]] QLineEdit *officialMirrorWidget() const { return officialMirror; }

    [[nodiscard]] bool pendingUpdate() const { return pending_update; }

    void pendingUpdate(const bool pending) { pending_update = pending; }

    void thirdMirrorUpdate(const QList<Mirrors::MirrorPlusEntry> &entries);

    void thirdMirrorUpdate(const QStringList &names);

    void populate() const;

signals:
    void mirrorChangeRequested(const QString &url);

    void reposEnable(const QList<Mirrors::MirrorPlusEntry> &entries);

    void reposRemoved(const QStringList &names);

    void officialBranchesChanged(const Mirrors::OfficialBranches &branches);

    void repoVerificationChanged(const RepoVerificationEntry &entry);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void onChangeClicked();

    void onAddCustomOfficialClicked();

    void onAddClicked();

    void onAddCustomThirdClicked();

    void onRemoveClicked();

    void onVerificationClicked();

    void updateSelectionButtons() const;

    QHBoxLayout *officialLayout{}, *officialBranchesLayout{}, *repositoryLayout{};
    QLineEdit *officialMirror{};
    QCheckBox *checkPasture{}, *checkTesting{}, *checkExtra{};
    QPushButton *btnChange{}, *btnAddCustomOfficial{};
    QPushButton *btnAdd{}, *btnAddCustomThird{}, *btnRemove{}, *btnVerification{};
    QStandardItemModel *model{};
    QVBoxLayout *buttonLayout{}, *mainLayout{};

    TableView *repositoryList{};

    bool pending_update{false};
};

#endif
