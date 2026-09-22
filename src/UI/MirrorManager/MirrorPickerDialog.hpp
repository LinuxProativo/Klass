/**
 * @file MirrorPickerDialog.hpp
 * @brief Header for selecting the official Slackware mirror with live latency testing.
 */

#ifndef MIRRORPICKERDIALOG_HPP
#define MIRRORPICKERDIALOG_HPP

#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTcpSocket>
#include <QTimer>
#include <QVBoxLayout>

#include <LatencySortModel.hpp>
#include <TableView.hpp>

/**
 * @class MirrorPickerDialog
 * @brief Dialog for selecting the official Slackware mirror.
 */
class MirrorPickerDialog final : public QDialog {
    Q_OBJECT

public:
    explicit MirrorPickerDialog(QWidget *parent = nullptr);

    [[nodiscard]] QString selectedMirror() const { return selectedUrl; }

protected:
    void showEvent(QShowEvent *event) override;

private:
    void onTestFinished(int row, qint64 ms, bool ok, QTcpSocket *socket);

    void populate() const;

    void testLatencies();

    void testOne(int row, const QString &url);

    void applySelection();

    QDialogButtonBox *buttons{};
    QHBoxLayout *statusLayout{};
    QLabel *statusLabel{};
    QPushButton *btnRetest{};
    QStandardItemModel *model{};
    QVBoxLayout *mainLayout{};

    LatencySortModel *proxy{};
    TableView *mirrors{};

    QString selectedUrl;
    int pendingTests{0};
};

#endif
