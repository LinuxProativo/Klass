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

#include <AdminConfigManager.hpp>
#include <HelperUtils.hpp>
#include <Mirrors.hpp>
#include <Packages.hpp>
#include <SlackwareDefines.hpp>
#include <Utils.hpp>

#include <gpgme.h>

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

                        if (bytesWritten >= 0 && !operationCancelled) { // NOLINT
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
            OUTPUT + QByteArray("\nUpdated DataBase Successful!") + SEP + QByteArray("UPDATE_DONE") + SEP);
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

    AdminConfigManager adminConfig;
    const bool checkChecksum = adminConfig.verifyChecksum();
    const bool checkSignature = adminConfig.verifySignature();

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

        const QString fileName = pkg.url.mid(pkg.url.lastIndexOf(u'/') + 1);
        socket->write(OUTPUT + QByteArray("\nProcessing ") + fileName.toUtf8() + "..." + SEP);

        if (checkChecksum && pkg.md5.trimmed().isEmpty()) {
            socket->write(ERROR + QByteArray("Verification required: Missing checksum for ")
                          + fileName.toUtf8() + ". Aborting." + SEP);
            operationCancelled = true;
            break;
        }

        if (checkSignature && pkg.ascUrl.trimmed().isEmpty()) {
            socket->write(ERROR + QByteArray("Verification required: Missing signature (.asc) for ")
                          + fileName.toUtf8() + ". Aborting." + SEP);
            operationCancelled = true;
            break;
        }

        const QString targetDir = KLASS_PACKAGES + pkg.repo + u'/' + pkg.category + u'/';
        (void) QDir().mkpath(targetDir);
        const QString savePath = targetDir + fileName;

        bool needDownloadPkg = true;
        if (QFile::exists(savePath)) {
            if (checkChecksum) {
                if (calculateLocalMd5(savePath) == pkg.md5) {
                    socket->write(
                        OUTPUT + QByteArray("    --> Package already downloaded and verified. Skipping.") + SEP);
                    needDownloadPkg = false;
                } else {
                    socket->write(
                        OUTPUT + QByteArray("    --> Local package checksum mismatch. Re-downloading.") + SEP);
                }
            } else {
                socket->write(OUTPUT + QByteArray("    --> Package already exists locally.") + SEP);
                needDownloadPkg = false;
            }
        }

        if (needDownloadPkg) {
            socket->write(OUTPUT + QByteArray("    --> Downloading package from: ") + pkg.url.toUtf8() + SEP);
            const auto res = Utils::download(QUrl(pkg.url), {}, [this, socket] {
                return checkSocketCancellation(socket, operationCancelled);
            });

            if (operationCancelled) break;

            if (!res.success) {
                socket->write(ERROR + QByteArray("Failed to download ") + fileName.toUtf8() + ". Aborting." + SEP);
                operationCancelled = true;
                break;
            }

            if (checkChecksum) {
                const auto downloadedHash =
                        QString(QCryptographicHash::hash(res.data, QCryptographicHash::Md5).toHex());
                if (downloadedHash != pkg.md5) {
                    socket->write(
                        ERROR + QByteArray("Failed to verify checksum for ") + fileName.toUtf8() + ". Aborting." + SEP);
                    operationCancelled = true;
                    break;
                }
                socket->write(OUTPUT + QByteArray("    --> Checksum successfully validated.") + SEP);
            }

            if (QFile f(savePath); f.open(QIODevice::WriteOnly)) {
                f.write(res.data);
                f.close();
            } else {
                socket->write(ERROR + QByteArray("Failed to save package file to disk. Aborting.") + SEP);
                operationCancelled = true;
                break;
            }
        }

        if (operationCancelled) break;

        if (checkSignature) {
            const QString ascName = pkg.ascUrl.mid(pkg.ascUrl.lastIndexOf(u'/') + 1);
            const QString ascPath = targetDir + ascName;

            bool needDownloadAsc = true;
            if (QFile::exists(ascPath)) {
                if (!pkg.ascMd5.isEmpty()) {
                    if (calculateLocalMd5(ascPath) == pkg.ascMd5)
                        needDownloadAsc = false;
                } else {
                    needDownloadAsc = false;
                }
            }

            if (needDownloadAsc) {
                socket->write(OUTPUT + QByteArray("    --> Downloading signature: ") + ascName.toUtf8() + SEP);
                const auto ascRes = Utils::download(QUrl(pkg.ascUrl), {}, [this, socket] {
                    return checkSocketCancellation(socket, operationCancelled);
                });

                if (operationCancelled) break; //NOLINT

                if (!ascRes.success) {
                    socket->write(
                        ERROR + QByteArray("No download signature for ") + fileName.toUtf8() + ". Aborting." + SEP);
                    operationCancelled = true;
                    break;
                }

                if (!pkg.ascMd5.isEmpty()) {
                    const auto ascHash = QString(
                        QCryptographicHash::hash(ascRes.data, QCryptographicHash::Md5).toHex());
                    if (ascHash != pkg.ascMd5) {
                        socket->write(
                            ERROR + QByteArray("MD5 mismatch for signature file ") + ascName.toUtf8() + ". Aborting." +
                            SEP);
                        operationCancelled = true;
                        break;
                    }
                }

                if (QFile f(ascPath); f.open(QIODevice::WriteOnly)) {
                    f.write(ascRes.data);
                    f.close();
                } else {
                    socket->write(ERROR + QByteArray("Failed to save signature to disk. Aborting.") + SEP);
                    operationCancelled = true;
                    break;
                }
            }

            socket->write(OUTPUT + QByteArray("    --> Verifying GPG signature...") + SEP);
            if (!verifyGpgSignature(ascPath, savePath, pkg.repo)) {
                socket->write(
                    ERROR + QByteArray("GPG signature check FAILED for ") + fileName.toUtf8() + ". Aborting." + SEP);
                operationCancelled = true;
                break;
            }
            socket->write(OUTPUT + QByteArray("    --> GPG signature valid and verified.") + SEP);
        }

        if (operationCancelled) break; //NOLINT

        if (pkg.action == QLatin1String("INSTALL") || pkg.action == QLatin1String("UPDATE")) {
            installList.append(savePath);
        } else if (pkg.action == QLatin1String("REINSTALL")) {
            reinstallList.append(savePath);
        }
    }

    if (operationCancelled) {
        socket->write(LOG + QByteArray("Transaction Cancelled due to verification or network error.") + SEP);
        return;
    }

    socket->write(OUTPUT + QByteArray("\n==================================================") + SEP);
    socket->write(OUTPUT + QByteArray("Executing Package Operations ...") + SEP);
    socket->write(OUTPUT + QByteArray("==================================================\n") + SEP);

    auto runTask = [&](const QString &cmd, const QStringList &args) {
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

    if (!installList.isEmpty() || !reinstallList.isEmpty() || !removeList.isEmpty()) {
        socket->write(OUTPUT + QByteArray("\n==================================================") + SEP);
        socket->write(OUTPUT + QByteArray("Running System Post-Configuration ...") + SEP);
        socket->write(OUTPUT + QByteArray("==================================================\n") + SEP);

        if (adminConfig.postInstallTask(PostInstallTask::Ldconfig) && QFile::exists("/sbin/ldconfig")) {
            socket->write(OUTPUT + QByteArray("Updating shared libraries cache (ldconfig)...") + SEP);
            runTask("/sbin/ldconfig", {});
        }

        if (adminConfig.postInstallTask(PostInstallTask::UpdateMimeDatabase) &&
            QFile::exists("/usr/bin/update-mime-database") && QDir("/usr/share/mime").exists()) {
            socket->write(OUTPUT + QByteArray("\nUpdating MIME database...") + SEP);
            runTask("/usr/bin/update-mime-database", {"/usr/share/mime"});
        }

        if (adminConfig.postInstallTask(PostInstallTask::UpdateManDb) && QFile::exists("/usr/bin/mandb")) {
            socket->write(OUTPUT + QByteArray("\nUpdating man page database (mandb)...") + SEP);
            runTask("/usr/bin/mandb", {"-q"});
        }

        if (adminConfig.postInstallTask(PostInstallTask::UpdateDesktopDatabase) &&
            QFile::exists("/usr/bin/update-desktop-database")) {
            socket->write(OUTPUT + QByteArray("\nUpdating desktop database...") + SEP);
            runTask("/usr/bin/update-desktop-database", {"-q"});
        }

        if (adminConfig.postInstallTask(PostInstallTask::UpdateGtkIconCache) &&
            QFile::exists("/usr/bin/gtk-update-icon-cache") &&
            QDir("/usr/share/icons/hicolor").exists()) {
            socket->write(OUTPUT + QByteArray("\nUpdating icon cache...") + SEP);
            runTask("/usr/bin/gtk-update-icon-cache", {"-q", "-t", "-f", "/usr/share/icons/hicolor"});
        }

        if (adminConfig.postInstallTask(PostInstallTask::CompileGlibSchemas) &&
            QFile::exists("/usr/bin/glib-compile-schemas") &&
            QDir("/usr/share/glib-2.0/schemas").exists()) {
            socket->write(OUTPUT + QByteArray("\nCompiling GLib schemas...") + SEP);
            runTask("/usr/bin/glib-compile-schemas", {"/usr/share/glib-2.0/schemas"});
        }

        if (adminConfig.postInstallTask(PostInstallTask::UpdateGrub) &&
            QFile::exists("/usr/sbin/grub-mkconfig")) {
            socket->write(OUTPUT + QByteArray("\nUpdating GRUB bootloader configuration...") + SEP);
            runTask("/usr/sbin/grub-mkconfig", {"-o", "/boot/grub/grub.cfg"});
        }

        if (adminConfig.postInstallTask(PostInstallTask::UpdateLilo) &&
            QFile::exists("/sbin/lilo") && QFile::exists("/etc/lilo.conf")) {
            socket->write(OUTPUT + QByteArray("\nUpdating LILO bootloader...") + SEP);
            runTask("/sbin/lilo", {});
        }

        if (adminConfig.postInstallTask(PostInstallTask::GenerateInitrd)) {
            if (QFile::exists("/usr/share/mkinitrd/mkinitrd_command_generator.sh")) {
                socket->write(OUTPUT + QByteArray("\nGenerating initrd for generic kernel...") + SEP);
                runTask("/usr/share/mkinitrd/mkinitrd_command_generator.sh", {"-r"});
            } else if (QFile::exists("/sbin/mkinitrd")) {
                socket->write(OUTPUT + QByteArray("\nRunning mkinitrd...") + SEP);
                runTask("/sbin/mkinitrd", {"-F"});
            }
        }

        if (adminConfig.postInstallTask(PostInstallTask::ReinstallVBoxModules)) {
            const QString vboxScript = QFile::exists("/etc/rc.d/rc.vboxdrv")
                                           ? "/etc/rc.d/rc.vboxdrv"
                                           : QFile::exists("/sbin/rc.vboxdrv")
                                                 ? "/sbin/rc.vboxdrv"
                                                 : QString{};
            if (!vboxScript.isEmpty()) {
                socket->write(OUTPUT + QByteArray("\nRebuilding VirtualBox kernel modules...") + SEP);
                runTask(vboxScript, {"setup"});
            }
        }
    }

    // NOLINTBEGIN
    if (operationCancelled) {
        socket->write(OUTPUT + QByteArray("Operations were interrupted.") + SEP);
        socket->write(LOG + QByteArray("Transaction Interrupted.") + SEP);
        return;
    }
    // NOLINTEND

    socket->write(OUTPUT + QByteArray("\nAll operations completed successfully!") + SEP);
    socket->write(LOG + QByteArray("Transaction Finished.") + SEP + QByteArray("TRANSACTION_DONE") + SEP);
}

/**
 * @brief Executes a detached OpenPGP signature verification using an active GPGME context.
 * @param ctx Valid GPGME context.
 * @param ascPath Absolute path to the .asc detached signature file.
 * @param pkgPath Absolute path to the target package archive.
 * @return True if the signature is valid, false otherwise.
 */
bool HelperUtils::executeGpgVerification(const gpgme_ctx_t *ctx, const QString &ascPath, const QString &pkgPath) {
    gpgme_data_t sigData = nullptr;
    gpgme_data_t textData = nullptr;

    if (gpgme_data_new_from_file(&sigData, ascPath.toLocal8Bit().constData(), 1) != GPG_ERR_NO_ERROR)
        return false;

    if (gpgme_data_new_from_file(&textData, pkgPath.toLocal8Bit().constData(), 1) != GPG_ERR_NO_ERROR) {
        gpgme_data_release(sigData);
        return false;
    }

    const gpgme_error_t err = gpgme_op_verify(*ctx, sigData, textData, nullptr);

    bool isValid = false;
    if (err == GPG_ERR_NO_ERROR) {
        if (const _gpgme_op_verify_result *result = gpgme_op_verify_result(*ctx); result && result->signatures) {
            isValid = result->signatures->status == GPG_ERR_NO_ERROR;
        }
    }

    gpgme_data_release(sigData);
    gpgme_data_release(textData);
    return isValid;
}

/**
 * @brief Imports a public GPG key file into the GPGME keyring.
 * @param ctx Valid GPGME context.
 * @param keyPath Absolute path to the public key file (e.g. GPG-KEY).
 * @return True if import succeeded, false otherwise.
 */
bool HelperUtils::importGpgKey(const gpgme_ctx_t *ctx, const QString &keyPath) {
    gpgme_data_t keyData = nullptr;
    if (gpgme_data_new_from_file(&keyData, keyPath.toLocal8Bit().constData(), 1) != GPG_ERR_NO_ERROR)
        return false;

    const gpgme_error_t err = gpgme_op_import(*ctx, keyData);
    gpgme_data_release(keyData);

    return (err == GPG_ERR_NO_ERROR);
}

/**
 * @brief Verifies the OpenPGP detached signature using the native GPGME C library.
 * @param ascPath Absolute path to the detached signature (.asc).
 * @param pkgPath Absolute path to the package archive.
 * @param repoName Name of the repository to look up its public GPG-KEY if needed.
 * @return True if the signature is valid, false otherwise.
 */
bool HelperUtils::verifyGpgSignature(const QString &ascPath, const QString &pkgPath, const QString &repoName) {
    static bool gpgmeInitialized = false;
    if (!gpgmeInitialized) {
        gpgme_check_version(nullptr);
        gpgmeInitialized = true;
    }

    gpgme_ctx_t ctx = nullptr;
    if (gpgme_new(&ctx) != GPG_ERR_NO_ERROR)
        return false;

    gpgme_set_protocol(ctx, GPGME_PROTOCOL_OpenPGP);

    bool isValid = executeGpgVerification(&ctx, ascPath, pkgPath);

    if (!isValid) {
        if (const QString repoKeyPath = KLASS_DATABASE + repoName + "/GPG-KEY";
            QFile::exists(repoKeyPath) && importGpgKey(&ctx, repoKeyPath)) {
            isValid = executeGpgVerification(&ctx, ascPath, pkgPath);
        }
    }

    gpgme_release(ctx);
    return isValid;
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
