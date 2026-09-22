/**
 * @file MirrorPickerDialog.cpp
 * @brief Dialog for selecting the official Slackware mirror with live latency testing.
 */

#include <MirrorPickerDialog.hpp>
#include <Mirrors.hpp>

#include <QDateTime>
#include <QHeaderView>
#include <QPushButton>

/**
 * @brief Constructs the mirror picker dialog.
 * @param parent Parent widget.
 */
MirrorPickerDialog::MirrorPickerDialog(QWidget *parent) : QDialog(parent) {
    this->setWindowTitle(tr("Select Mirror"));
    this->setMinimumSize(672, 378);

    statusLabel = new QLabel(tr("Testing latency..."), this);
    btnRetest = new QPushButton(tr("Reload Test"), this);
    btnRetest->setIcon(QIcon::fromTheme("view-refresh"));
    btnRetest->setToolTip(tr("Retest latency"));
    btnRetest->setEnabled(false);
    connect(btnRetest, &QPushButton::clicked, this, [this] {
        btnRetest->setEnabled(false);
        statusLabel->setText(tr("Testing latency..."));
        testLatencies();
    });

    statusLayout = new QHBoxLayout();
    statusLayout->addWidget(btnRetest);
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();

    model = new QStandardItemModel(0, 3, this);
    model->setHorizontalHeaderLabels({tr("Country"), tr("URL"), tr("Latency")});

    proxy = new LatencySortModel(this);
    proxy->setSourceModel(model);

    mirrors = new TableView(this);
    mirrors->setModel(proxy);
    mirrors->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mirrors->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    mirrors->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    mirrors->horizontalHeader()->setStretchLastSection(false);
    mirrors->setSelectionMode(QAbstractItemView::SingleSelection);
    mirrors->sortByColumn(2, Qt::AscendingOrder);

    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(buttons, &QDialogButtonBox::accepted, this, &MirrorPickerDialog::applySelection);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(mirrors->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection &s) {
        buttons->button(QDialogButtonBox::Ok)->setEnabled(!s.isEmpty());
    });

    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(statusLayout);
    mainLayout->addWidget(mirrors);
    mainLayout->addWidget(buttons);

    populate();
    testLatencies();
}

/**
 * @brief Handles cleanup and model updates when a mirror latency test completes or times out.
 * @param row Table row index.
 * @param ms Measured latency in milliseconds.
 * @param ok True if connection succeeded, false on error or timeout.
 * @param socket Pointer to the socket used for testing.
 */
void MirrorPickerDialog::onTestFinished(const int row, const qint64 ms, const bool ok, QTcpSocket *socket) {
    if (!socket) return;
    socket->abort();
    socket->deleteLater();

    if (auto *item = model->item(row, 2)) {
        if (ok) {
            item->setText(QString::number(ms) + QStringLiteral(" ms"));
            item->setData(ms, Qt::UserRole);
        } else {
            item->setText(tr("timeout"));
            item->setData(99999, Qt::UserRole);
        }
    }

    if (--pendingTests == 0) {
        statusLabel->setText(tr("Latency test complete."));
        btnRetest->setEnabled(true);
    }
}

/**
 * @brief Populates the table with all mirrors for the current version/arch.
 */
void MirrorPickerDialog::populate() const {
    model->setRowCount(0);
    const QString activeUrl = Mirrors::activeOne().url;

    for (const auto &[url, country]: Mirrors::listAll()) {
        auto *countryItem = new QStandardItem(country); // NOLINT
        auto *latencyItem = new QStandardItem(tr("testing")); // NOLINT
        latencyItem->setTextAlignment(Qt::AlignCenter);
        latencyItem->setData(99998, Qt::UserRole);

        model->appendRow({countryItem, new QStandardItem(url), latencyItem});

        if (url == activeUrl) {
            countryItem->setData(true, Qt::UserRole + 1);
            const QModelIndex srcIdx = model->index(model->rowCount() - 1, 0);
            const QModelIndex proxyIdx = proxy->mapFromSource(srcIdx);
            mirrors->setCurrentIndex(proxyIdx);
        }
    }
}

/**
 * @brief Starts a TCP latency test for every row.
 */
void MirrorPickerDialog::testLatencies() {
    pendingTests = model->rowCount();
    if (pendingTests == 0) {
        statusLabel->setText(tr("No mirrors found."));
        return;
    }

    for (int row = 0; row < model->rowCount(); ++row)
        testOne(row, model->item(row, 1)->text());
}

/**
 * @brief Opens a TCP connection to the mirror host and measures time to connect.
 * @param row Table row index.
 * @param url Mirror URL to test.
 */
void MirrorPickerDialog::testOne(const int row, const QString &url) {
    const QUrl p(url);
    const QString host = p.host();
    const QString s = p.scheme().toLower();
    const auto port = static_cast<quint16>(p.port() > 0 ? p.port() : s == "ftp" ? 21 : s == "https" ? 443 : 80);

    auto *socket = new QTcpSocket(this); // NOLINT
    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    connect(socket, &QTcpSocket::connected, this, [this, socket, startTime, row] {
        const qint64 ms = QDateTime::currentMSecsSinceEpoch() - startTime;
        onTestFinished(row, ms, true, socket);
    });

    connect(socket, &QAbstractSocket::errorOccurred, this, [this, socket, row](QAbstractSocket::SocketError) {
        onTestFinished(row, 0, false, socket);
    });

    QTimer::singleShot(3000, socket, [this, socket, row] {
        if (socket->state() != QAbstractSocket::ConnectedState)
            onTestFinished(row, 0, false, socket);
    });

    socket->connectToHost(host, port);
}

/**
 * @brief Stores the chosen mirror URL and closes the dialog with Accepted.
 */
void MirrorPickerDialog::applySelection() {
    const QModelIndex proxyIdx = mirrors->currentIndex();
    if (!proxyIdx.isValid()) {
        reject();
        return;
    }

    const QModelIndex srcIdx = proxy->mapToSource(proxyIdx);
    selectedUrl = model->item(srcIdx.row(), 1)->text();
    accept();
}

/**
 * @brief Scrolls to the selected row once the dialog is fully visible.
 * @param event The show event instance.
 */
void MirrorPickerDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    mirrors->scrollToTop();
}
