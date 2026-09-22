/**
 * @file MirrorManager.hpp
 * @brief Header for configure mirrors using MirrorManager class.
 */

#ifndef MIRRORMANAGER_HPP
#define MIRRORMANAGER_HPP

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <Dialog.hpp>
#include <Mirrors.hpp>
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

protected:
    void showEvent(QShowEvent *event) override;

private:
    void onChangeClicked();

    void onAddCustomOfficialClicked();

    void onAddClicked();

    void onAddCustomThirdClicked();

    void onRemoveClicked();

    QHBoxLayout *officialLayout{}, *repositoryLayout{};
    QLineEdit *officialMirror{};
    QPushButton *btnChange{}, *btnAddCustomOfficial{}, *btnAdd{}, *btnAddCustomThird{}, *btnRemove{};
    QStandardItemModel *model{};
    QVBoxLayout *buttonLayout{}, *mainLayout{};

    TableView *repositoryList{};

    bool pending_update{false};
};

#endif
