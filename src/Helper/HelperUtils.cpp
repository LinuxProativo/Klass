/**
 * @file HelperUtils.cpp
 * Implementation of helper utilities for network operations and database updates.
 */

#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#include <HelperUtils.hpp>
#include <Mirrors.hpp>
#include <Packages.hpp>
#include <SlackwareDefines.hpp>
#include <Utils.hpp>

/**
 * @brief Executes the update process for the database.
 * @param socket A pointer to the QLocalSocket used for bidirectional communication.
 */
void HelperUtils::performUpdate(QLocalSocket *socket) {
    socket->write(OUTPUT + QByteArray("Starting Update DataBase...") + SEP);
    (void) QDir().mkpath(KLASS_DATABASE);

    const QStringList filesToDownload = {
        "CHECKSUMS.md5.gz.asc", "CHECKSUMS.md5.gz", "ChangeLog.txt", "GPG-KEY", "PACKAGES.TXT.gz"
    }; // FILELIST.TXT não vi vantagem

    QList<Mirrors::MirrorPlusEntry> repos;

    if (const auto [url, country] = Mirrors::activeOne(); !url.isEmpty()) {
        repos.append({SLACK_OFICIAL, url});
        repos.append({SLACK_PATCHES, url + "patches/"});
        repos.append({SLACK_EXTRA, url + "extra/"});
        repos.append({SLACK_TESTING, url + "testing/"});
    }
    repos.append(Mirrors::listActive());

    for (const auto &[name, url]: repos) {
        socket->waitForReadyRead(0);
        if (operationCancelled) break;

        (void) QDir().mkpath(KLASS_DATABASE + name);
        socket->write(OUTPUT + QByteArray("\nUpdating Mirror ") + name.toUtf8() + " : " + url.toUtf8() + SEP);

        for (const QString &fileName: filesToDownload) {
            if (socket->readAll().contains("CANCEL_OPERATION")) // Só por garantia mesmo
                operationCancelled = true;

            QStringList targets{};
            if (fileName.startsWith("ChangeLog")) {
                if (!name.contains(SLACK_PATCHES) && !name.contains(SLACK_EXTRA) && !name.contains(SLACK_TESTING)) {
                    targets << fileName;
                    targets << "../" + fileName;
                    targets << "../../" + fileName;
                }
            } else if (fileName.endsWith(".gz.asc")) {
                targets << fileName;
                targets << fileName.left(fileName.length() - 6) + "asc";
            } else if (fileName.endsWith(".gz")) {
                targets << fileName;
                targets << fileName.left(fileName.length() - 3);
            } else
                targets << fileName;

            bool success = false;
            for (QString &targetFile: targets) {
                const QString targetUrl = url + (url.endsWith('/') ? "" : "/") + targetFile;
                const auto res = Utils::download(QUrl(targetUrl), {}, [this, socket] {
                    return checkSocketCancellation(socket, operationCancelled);
                });

                if (operationCancelled) break;

                if (res.success) {
                    if (fileName.startsWith("ChangeLog"))
                        targetFile = QString("ChangeLog.txt");

                    const QString savePath = KLASS_DATABASE + name + "/" + targetFile;
                    if (QFile file(savePath); file.open(QIODevice::WriteOnly)) {
                        const qint64 bytesWritten = file.write(res.data);
                        file.close();

                        if (bytesWritten >= 0) {
                            success = true;
                            socket->write(OUTPUT + QByteArray("    --> Download File ") + targetUrl.toUtf8() + SEP);
                        }
                    }
                } else {
                    const auto msg = "HTTP StatusCode [" + QString::number(res.statusCode) + "] Code: " +
                                     QString::number(res.error);
                    QTextStream(stdout) << msg;
                }
                if (success) break;
            }
            if (operationCancelled) break;
        }
    }

    if (!operationCancelled) {
        socket->write(
            OUTPUT + QByteArray("\nUpdated DataBase Successful!") + SEP + QByteArray("DONE") + SEP +
            QByteArray("UPDATE_DONE") + SEP);
    } else {
        socket->write(LOG + QByteArray("Cancelled Update.") + SEP);
    }
}

/**
 * @brief Processes the download and validation of packages for a transaction.
 * @param socket Communication socket to GUI.
 * @param task TaskManager instance for dispatching final installation commands.
 * @param pkgs List of packages to be processed.
 */
void HelperUtils::processTransaction(QLocalSocket *socket, TaskManager *task, const QList<TransactionPkg> &pkgs) {
    socket->write(OUTPUT + QByteArray("Starting transaction processing...") + SEP);
    QStringList installList, reinstallList, removeList;

    for (const auto &pkg: pkgs) {
        if (socket->readAll().contains("CANCEL_OPERATION")) {
            operationCancelled = true;
            break;
        }

        if (pkg.action == QLatin1String("REMOVE")) {
            removeList.append(pkg.name);
            continue;
        }

        if (pkg.url.isEmpty()) continue;

        const QString targetDir = KLASS_PACKAGES + pkg.repo + u'/' + pkg.category + u'/';
        (void) QDir().mkpath(targetDir);

        const QString fileName = pkg.url.mid(pkg.url.lastIndexOf(u'/') + 1);
        const QString savePath = targetDir + fileName;
        socket->write(OUTPUT + QByteArray("\nProcessing ") + fileName.toUtf8() + "..." + SEP);

        bool needDownloadPkg = true;
        if (QFile::exists(savePath) && !pkg.md5.isEmpty()) {
            if (calculateLocalMd5(savePath) == pkg.md5) {
                socket->write(OUTPUT + QByteArray("    --> Package already downloaded and verified. Skipping.") + SEP);
                needDownloadPkg = false;
            } else {
                socket->write(OUTPUT + QByteArray("    --> Incomplete package. Re-downloading.") + SEP);
            }
        }

        if (needDownloadPkg) {
            const auto res = Utils::download(QUrl(pkg.url), {}, [this, socket] {
                return checkSocketCancellation(socket, operationCancelled);
            });

            if (operationCancelled) break;

            if (res.success) {
                if (QString(QCryptographicHash::hash(res.data, QCryptographicHash::Md5).toHex()) != pkg.md5) {
                    socket->write(ERROR + QByteArray("MD5 mismatch for ") + fileName.toUtf8() + ". Aborting." + SEP);
                    operationCancelled = true;
                    break;
                }

                if (QFile f(savePath); f.open(QIODevice::WriteOnly)) {
                    f.write(res.data);
                    f.close();
                    socket->write(OUTPUT + QByteArray("    --> Download complete and MD5 validated.") + SEP);
                }
            } else {
                socket->write(ERROR + QByteArray("Failed to download ") + fileName.toUtf8() + SEP);
                operationCancelled = true;
                break;
            }
        }

        if (operationCancelled) break;

        if (pkg.action == QLatin1String("INSTALL") || pkg.action == QLatin1String("UPDATE")) {
            installList.append(savePath);
        } else if (pkg.action == QLatin1String("REINSTALL")) {
            reinstallList.append(savePath);
        }

        if (!pkg.ascUrl.isEmpty()) {
            const QString ascName = pkg.ascUrl.mid(pkg.ascUrl.lastIndexOf(u'/') + 1);
            const QString ascPath = targetDir + ascName;

            bool needDownloadAsc = true;
            if (QFile::exists(ascPath) && !pkg.ascMd5.isEmpty()) {
                if (calculateLocalMd5(ascPath) == pkg.ascMd5) {
                    socket->write(OUTPUT + QByteArray("    --> Signature already exists and verified.") + SEP);
                    needDownloadAsc = false;
                }
            }

            if (needDownloadAsc) {
                const auto ascRes = Utils::download(QUrl(pkg.ascUrl), {}, [this] { return operationCancelled; });
                if (operationCancelled) break; // NOLINT
                if (ascRes.success) {
                    if (QString(QCryptographicHash::hash(ascRes.data, QCryptographicHash::Md5).toHex()) != pkg.ascMd5) {
                        socket->write(ERROR + QByteArray("MD5 mismatch for signature ") + ascName.toUtf8() + SEP);
                    } else if (QFile f(ascPath); f.open(QIODevice::WriteOnly)) {
                        f.write(ascRes.data);
                        f.close();
                        socket->write(OUTPUT + QByteArray("    --> Signature downloaded and validated.") + SEP);
                    }
                }
            }
        }
    }

    if (operationCancelled) {
        socket->write(LOG + QByteArray("Transaction Cancelled.") + SEP);
        return;
    }

    socket->write(OUTPUT + QByteArray("\n==================================================") + SEP);
    socket->write(OUTPUT + QByteArray("Executing Package Operations ...") + SEP);
    socket->write(OUTPUT + QByteArray("==================================================\n") + SEP);

    auto runTask = [&](const QString &cmd, const QStringList &args) {
        if (args.isEmpty() || operationCancelled) return;

        QEventLoop loop;
        connect(task, &TaskManager::commandFinished, &loop, &QEventLoop::quit);

        task->startCommand(cmd, args);
        loop.exec();
    };

    if (!removeList.isEmpty()) {
        runTask("/sbin/removepkg", removeList);
    }
    if (!installList.isEmpty()) {
        QStringList args;
        args << "--install-new" << installList;
        runTask("/sbin/upgradepkg", args);
    }
    if (!reinstallList.isEmpty()) {
        QStringList args;
        args << "--install-new" << "--reinstall" << reinstallList;
        runTask("/sbin/upgradepkg", args);
    }

    // NOLINTBEGIN
    if (operationCancelled) {
        socket->write(OUTPUT + QByteArray("Operations were interrupted.") + SEP);
        socket->write(LOG + QByteArray("Transaction Interrupted.") + SEP);
        return;
    }
    // NOLINTEND

    socket->write(OUTPUT + QByteArray("\nAll operations completed successfully!") + SEP);
    socket->write(LOG + QByteArray("Transaction Finished.") + SEP);
    socket->write(QByteArray("TRANSACTION_DONE") + SEP + QByteArray("DONE") + SEP);
}

/**
 * @brief Calculates the MD5 hash of a local file.
 * @param filePath The full path to the file.
 * @return A string containing the hexadecimal MD5 hash, or empty if it fails.
 */
QString HelperUtils::calculateLocalMd5(const QString &filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};

    if (QCryptographicHash hash(QCryptographicHash::Md5); hash.addData(&f))
        return hash.result().toHex();

    return {};
}

/**
 * @brief Checks if the socket connection has been interrupted or if a cancellation flag is set.
 * @param socket Pointer to the QLocalSocket instance to check.
 * @param cancelledFlag Output reference set to true if cancellation or disconnection occurs.
 * @return true if the operation is canceled or the socket is disconnected/invalid; false otherwise.
 */
bool HelperUtils::checkSocketCancellation(QLocalSocket *socket, bool &cancelledFlag) {
    if (cancelledFlag) return true;

    if (socket) {
        socket->waitForReadyRead(0);

        if (const QByteArray buffer = socket->readAll(); buffer.contains("CANCEL_OPERATION")) {
            cancelledFlag = true;
            return true;
        }
    }
    return false;
}
