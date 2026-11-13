/**
 * @file NewConfigDialog.hpp
 * @brief Header for the interactive Slackware .new configuration resolution dialog.
 */

#ifndef NEWCONFIGDIALOG_HPP
#define NEWCONFIGDIALOG_HPP

#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QThread>
#include <QVBoxLayout>

#include <Dialog.hpp>
#include <DiffEngine.hpp>
#include <HelperController.hpp>
#include <NewConfigManager.hpp>
#include <NewConfigWorker.hpp>
#include <TableView.hpp>

/**
 * @class NewConfigDialog
 * @brief Dialog window allowing file-by-file or bulk review and resolution of .new files.
 */
class NewConfigDialog final : public Dialog {
    Q_OBJECT

public:
    explicit NewConfigDialog(const QList<NewConfigFileItem> &items, HelperController *helperC, QWidget *parent = {});

    ~NewConfigDialog() override;

    [[nodiscard]] QList<NewConfigFileItem> resolvedItems() const { return fileItems; }

    void updateFileList(const QList<NewConfigFileItem> &newItems);

signals:
    void applyResolutionRequested(const QList<NewConfigFileItem> &items);

    void requestDiffComputation(quint64 requestId, const QString &oldPath, const QString &newPath);

private slots:
    void onSelectionChanged();

    void onDiffComputed(quint64 requestId, const QString &oldPath, const QString &newPath,
                        const QList<DiffEngine::DiffLine> &diffLines, bool oldExists, bool permissionDenied) const;

    void onApplyClicked() { emit applyResolutionRequested(resolvedItems()); }

    void onReadAsRootClicked();

    void onRootConfigDataReceived(const QString &oldText, const QString &newText, const QString &oldPath,
                                  const QString &newPath);

private:
    void populateTable();

    void setActForSelected(NewConfigAction action);

    void setActionForAll(NewConfigAction action);

    void applyDiffToView(const QList<DiffEngine::DiffLine> &diffLines) const;

    void setDiffHeader(const QString &oldPath, const QString &newP, bool isRoot = false, bool oldExists = true) const;

    void clearDiffHeader() const;

    QHBoxLayout *bulkLayout, *itemActionsLayout{}, *headerLayout{}, *bottomLayout{};
    QLabel *lblDiffTitle{}, *lblOldPath{}, *lblNewPath{};
    QPushButton *btnKeepAll{}, *btnOverwriteAll{}, *btnRemoveAll{}, *btnReadAsRoot{};
    QPushButton *btnKeepSelected{}, *btnOverwriteSel{}, *btnRemoveSelected{}, *btnApply{}, *btnClose{};
    QSplitter *splitter{};
    QStandardItemModel *tableModel{};
    QTextEdit *diffView{};
    QVBoxLayout *mainLayout{}, *leftLayout{}, *pathsLayout{}, *rightLayout{};
    QWidget *leftContainer{}, *rightContainer{};

    HelperController *helper{};
    NewConfigWorker *diffWorker{};
    TableView *fileTable{};

    QHash<QString, QList<DiffEngine::DiffLine> > rootDiffCache{};
    QList<NewConfigFileItem> fileItems{};
    QString currentSelectedOldPath{}, currentSelectedNewPath{};
    quint64 currentRequestId{0};
    QThread workerThread{};
};

#endif
