/**
 * @file NewConfigDialog.hpp
 * @brief Dialog window for reviewing and resolving Slackware .new configuration files.
 */

#ifndef NEWCONFIGDIALOG_HPP
#define NEWCONFIGDIALOG_HPP

#include <QAction>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QVBoxLayout>

#include <Dialog.hpp>
#include <NewConfigManager.hpp>
#include <TableView.hpp>

/**
 * @class NewConfigDialog
 * @brief Interactive dialog for resolving .new configuration files with built-in diff viewer.
 */
class NewConfigDialog final : public Dialog {
    Q_OBJECT

public:
    explicit NewConfigDialog(const QList<NewConfigFileItem> &items, QWidget *parent = nullptr);
    ~NewConfigDialog() override;

    [[nodiscard]] QList<NewConfigFileItem> resolvedItems() const;

signals:
    void applyResolutionRequested(const QList<NewConfigFileItem> &items);

private:
    void setupUi();
    void populateTable();
    void onSelectionChanged();
    void setActionForSelected(NewConfigAction action);
    void setActionForAll(NewConfigAction action);
    void renderDiff(const QString &oldPath, const QString &newPath);

    QList<NewConfigFileItem> fileItems;

    TableView *fileTable{};
    QStandardItemModel *tableModel{};
    QTextEdit *diffView{};
    QLabel *diffHeaderLabel{};

    QPushButton *btnKeepAll{};
    QPushButton *btnOverwriteAll{};
    QPushButton *btnRemoveAll{};

    QPushButton *btnKeepSelected{};
    QPushButton *btnOverwriteSelected{};
    QPushButton *btnRemoveSelected{};

    QPushButton *btnApply{};
    QPushButton *btnClose{};

    QSplitter *splitter{};
    QVBoxLayout *mainLayout{};
};

#endif // NEWCONFIGDIALOG_HPP