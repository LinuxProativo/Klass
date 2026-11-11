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
    explicit NewConfigDialog(const QList<NewConfigFileItem> &items,
                             HelperController *helperController,
                             QWidget *parent = nullptr);
    ~NewConfigDialog() override;

    [[nodiscard]] QList<NewConfigFileItem> resolvedItems() const;
    void updateFileList(const QList<NewConfigFileItem> &newItems);

signals:
    void applyResolutionRequested(const QList<NewConfigFileItem> &items);
    void requestDiffComputation(quint64 requestId, const QString &oldPath, const QString &newPath);

private slots:
    void onSelectionChanged();
    void onDiffComputed(quint64 requestId, const QString &oldPath, const QString &newPath,
                        const QList<DiffEngine::DiffLine> &diffLines, bool oldExists, bool permissionDenied) const;
    void onApplyClicked();
    void onReadAsRootClicked();
    void onRootConfigDataReceived(const QString &oldText, const QString &newText,
                                  const QString &oldPath, const QString &newPath);

private:
    void setupUi();
    void populateTable();
    void setActionForSelected(NewConfigAction action);
    void setActionForAll(NewConfigAction action);
    void applyDiffToView(const QList<DiffEngine::DiffLine> &diffLines) const;

    QList<NewConfigFileItem> fileItems;
    HelperController *helper{nullptr};

    QHash<QString, QList<DiffEngine::DiffLine>> rootDiffCache;

    TableView *fileTable{};
    QStandardItemModel *tableModel{};
    QTextEdit *diffView{};
    QLabel *diffHeaderLabel{};

    QPushButton *btnKeepAll{};
    QPushButton *btnOverwriteAll{};
    QPushButton *btnRemoveAll{};
    QPushButton *btnReadAsRoot{};

    QPushButton *btnKeepSelected{};
    QPushButton *btnOverwriteSelected{};
    QPushButton *btnRemoveSelected{};

    QPushButton *btnApply{};
    QPushButton *btnClose{};

    QSplitter *splitter{};
    QVBoxLayout *mainLayout{};

    QThread workerThread;
    NewConfigWorker *diffWorker{};
    quint64 currentRequestId{0};
    QString currentSelectedOldPath{};
    QString currentSelectedNewPath{};
};

#endif
