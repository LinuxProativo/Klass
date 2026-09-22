/**
 * @file RepoPlusPickerDialog.hpp
 * @brief Header for adding third-party Slackpkg+ repositories.
 */

#ifndef REPOPLUSPICKERDIALOG_HPP
#define REPOPLUSPICKERDIALOG_HPP

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <Mirrors.hpp>
#include <TableView.hpp>

/**
 * @class RepoPlusPickerDialog
 * @brief Dialog for adding third-party Slackpkg+ repositories.
 */
class RepoPlusPickerDialog final : public QDialog {
    Q_OBJECT

public:
    explicit RepoPlusPickerDialog(QWidget *parent = nullptr);

    [[nodiscard]] QList<Mirrors::MirrorPlusEntry> selectedRepos() const { return selectedEntries; }

private:
    void populate() const;

    void applySelection();

    QDialogButtonBox *buttonBox{};
    QLabel *infoLabel{};
    QStandardItemModel *model{};
    QVBoxLayout *mainLayout{};

    TableView *repoTable{};

    QList<Mirrors::MirrorPlusEntry> selectedEntries{};
};

#endif
