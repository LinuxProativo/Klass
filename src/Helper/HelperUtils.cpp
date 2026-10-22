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
#include <RepoVerificationManager.hpp>
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

        const auto branches = Mirrors::loadOfficialBranches();
        if (branches.pasture)
            repos.append({SLACK_PASTURE, url + "pasture/"});
        if (branches.extra)
            repos.append({SLACK_EXTRA, url + "extra/"});
        if (branches.testing)
            repos.append({SLACK_TESTING, url + "testing/"});
    }
    repos.append(Mirrors::listActive());

    AdminConfigManager adminConfig;
    const bool globalVerifySignature = adminConfig.verifySignature();

    for (const auto &[name, url]: repos) {
        socket->waitForReadyRead(0);
        if (operationCancelled) break;

        (void) QDir().mkpath(KLASS_DATABASE + name);
        socket->write(OUTPUT + QByteArray("\nUpdating Mirror ") + name.toUtf8() + " : " + url.toUtf8() + SEP);

        const bool isOfficialRepo = (name == SLACK_OFICIAL || name == SLACK_PATCHES ||
                                     name == SLACK_EXTRA || name == SLACK_TESTING ||
                                     name == SLACK_PASTURE);

        for (const QString &fileName: filesToDownload) {
            if (socket->readAll().contains("CANCEL_OPERATION")) // Só por garantia mesmo
                operationCancelled = true;

            QStringList targets{};
            if (fileName.startsWith("ChangeLog")) {
                if (!name.contains(SLACK_PATCHES) && !name.contains(SLACK_EXTRA) &&
                    !name.contains(SLACK_TESTING) && !name.contains(SLACK_PASTURE)) {
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

        bool checkChecksumSig = false;
        if (globalVerifySignature) {
            if (isOfficialRepo) {
                checkChecksumSig = true;
            } else {
                const auto repoVerif = RepoVerificationManager::getVerification(name);
                checkChecksumSig = repoVerif.verifyChecksumSig;
            }
        }

        if (checkChecksumSig && !operationCancelled) { //NOLINT
            const QString dbDir = KLASS_DATABASE + name + u'/';
            QString checksumFile, checksumAscFile;

            if (QFile::exists(dbDir + "CHECKSUMS.md5.gz"))
                checksumFile = dbDir + "CHECKSUMS.md5.gz";
            else if (QFile::exists(dbDir + "CHECKSUMS.md5"))
                checksumFile = dbDir + "CHECKSUMS.md5";

            if (QFile::exists(dbDir + "CHECKSUMS.md5.gz.asc"))
                checksumAscFile = dbDir + "CHECKSUMS.md5.gz.asc";
            else if (QFile::exists(dbDir + "CHECKSUMS.md5.asc"))
                checksumAscFile = dbDir + "CHECKSUMS.md5.asc";

            if (checksumFile.isEmpty() || checksumAscFile.isEmpty()) {
                socket->write(
                    ERROR + QByteArray("Verification required: Missing CHECKSUMS or signature (.asc) file for mirror ")
                    + name.toUtf8() + ". Purging directory and aborting update." + SEP);
                QDir(dbDir).removeRecursively();
                operationCancelled = true;
                break;
            }

            socket->write(OUTPUT + QByteArray("    --> Verifying CHECKSUMS GPG signature...") + SEP);
            if (!verifyGpgSignature(checksumAscFile, checksumFile, name)) {
                socket->write(ERROR + QByteArray("CHECKSUMS signature verification FAILED for mirror ")
                              + name.toUtf8() + ". Purging corrupted directory and aborting update." + SEP);
                QDir(dbDir).removeRecursively();
                operationCancelled = true;
                break;
            }
            socket->write(OUTPUT + QByteArray("    --> CHECKSUMS signature valid and verified.") + SEP);
        }

        if (operationCancelled) break; //NOLINT
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
    bool kernelOperated = false;

    AdminConfigManager adminConfig;
    const bool globalVerifyChecksum = adminConfig.verifyChecksum();
    const bool globalVerifySignature = adminConfig.verifySignature();

    for (const auto &pkg: pkgs) {
        if (socket->readAll().contains("CANCEL_OPERATION")) {
            operationCancelled = true;
            break;
        }

        if (pkg.name.startsWith(QLatin1String("kernel-"), Qt::CaseInsensitive))
            kernelOperated = true;

        if (pkg.action == QLatin1String("REMOVE")) {
            removeList.append(pkg.name);
            continue;
        }

        if (pkg.url.isEmpty()) continue;

        const QString fileName = pkg.url.mid(pkg.url.lastIndexOf(u'/') + 1);
        socket->write(OUTPUT + QByteArray("\nProcessing ") + fileName.toUtf8() + "..." + SEP);

        const bool isOfficialRepo = pkg.repo == SLACK_OFICIAL || pkg.repo == SLACK_PATCHES ||
                                    pkg.repo == SLACK_EXTRA || pkg.repo == SLACK_TESTING || pkg.repo == SLACK_PASTURE;

        bool checkChecksum = false;
        bool checkSignature = false;

        if (isOfficialRepo) {
            checkChecksum = globalVerifyChecksum;
            checkSignature = globalVerifySignature;
        } else {
            const auto verif = RepoVerificationManager::getVerification(pkg.repo);
            checkChecksum = globalVerifyChecksum && verif.verifyChecksum;
            checkSignature = globalVerifySignature && verif.verifySig;
        }

        if (checkChecksum && pkg.md5.trimmed().isEmpty()) {
            socket->write(
                OUTPUT + QByteArray("ERROR: Missing checksum for ") + fileName.toUtf8() + ". Aborting." + SEP +
                QByteArray("TRANSACTION_DONE") + SEP);
            operationCancelled = true;
            break;
        }

        if (checkSignature && pkg.ascUrl.trimmed().isEmpty()) {
            socket->write(
                OUTPUT + QByteArray("ERROR: Missing signature (.asc) for ") + fileName.toUtf8() + ". Aborting." + SEP +
                QByteArray("TRANSACTION_DONE") + SEP);
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
                socket->write(
                    OUTPUT + QByteArray("ERROR: Failed to download ") + fileName.toUtf8() + ". Aborting." + SEP +
                    QByteArray("TRANSACTION_DONE") + SEP);
                operationCancelled = true;
                break;
            }

            if (checkChecksum) {
                const auto downloadedHash =
                        QString(QCryptographicHash::hash(res.data, QCryptographicHash::Md5).toHex());
                if (downloadedHash != pkg.md5) {
                    socket->write(
                        OUTPUT + QByteArray("ERROR: Failed to verify checksum for ") + fileName.toUtf8() + ". Aborting."
                        + SEP + QByteArray("TRANSACTION_DONE") + SEP);
                    operationCancelled = true;
                    break;
                }
                socket->write(OUTPUT + QByteArray("    --> Checksum successfully validated.") + SEP);
            }

            if (QFile f(savePath); f.open(QIODevice::WriteOnly)) {
                f.write(res.data);
                f.close();
            } else {
                socket->write(
                    OUTPUT + QByteArray("ERROR: Failed to save package file to disk. Aborting.") + SEP + QByteArray(
                        "TRANSACTION_DONE") + SEP);
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
                        OUTPUT + QByteArray("ERROR: No download signature for ") + fileName.toUtf8() + ". Aborting." +
                        SEP + QByteArray("TRANSACTION_DONE") + SEP);
                    operationCancelled = true;
                    break;
                }

                if (!pkg.ascMd5.isEmpty()) {
                    const auto ascHash = QString(
                        QCryptographicHash::hash(ascRes.data, QCryptographicHash::Md5).toHex());
                    if (ascHash != pkg.ascMd5) {
                        socket->write(
                            OUTPUT + QByteArray("ERROR: MD5 mismatch for signature file ") + ascName.toUtf8() +
                            ". Aborting." + SEP + QByteArray("TRANSACTION_DONE") + SEP);
                        operationCancelled = true;
                        break;
                    }
                }

                if (QFile f(ascPath); f.open(QIODevice::WriteOnly)) {
                    f.write(ascRes.data);
                    f.close();
                } else {
                    socket->write(
                        OUTPUT + QByteArray("ERROR: Failed to save signature to disk. Aborting.") + SEP + QByteArray(
                            "TRANSACTION_DONE") + SEP);
                    operationCancelled = true;
                    break;
                }
            }

            socket->write(OUTPUT + QByteArray("    --> Verifying GPG signature...") + SEP);
            if (!verifyGpgSignature(ascPath, savePath, pkg.repo)) {
                socket->write(
                    OUTPUT + QByteArray("ERROR: GPG signature check FAILED for ") + fileName.toUtf8() + ". Aborting." +
                    SEP + QByteArray("TRANSACTION_DONE") + SEP);
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

        if (kernelOperated) {
            if (adminConfig.postInstallTask(PostInstallTask::GenerateInitrd) &&
                QFile::exists("/usr/share/mkinitrd/mkinitrd_command_generator.sh")) {
                socket->write(OUTPUT + QByteArray("\nGenerating initrd for generic kernel (with fallback protection)...") + SEP);
                const auto initrdCmd = QStringLiteral(
                    "if [ -e /boot/initrd.gz ]; then "
                    "  cp -L /boot/initrd.gz /boot/initrd.gz.bak; "
                    "fi; "
                    "if /usr/share/mkinitrd/mkinitrd_command_generator.sh | bash; then "
                    "  if [ -f /boot/initrd.gz ]; then "
                    "    KVER=$(uname -r); "
                    "    mv /boot/initrd.gz /boot/initrd-${KVER}.gz && "
                    "    ln -sf initrd-${KVER}.gz /boot/initrd.gz && "
                    "    rm -f /boot/initrd.gz.bak; "
                    "  fi; "
                    "else "
                    "  if [ -f /boot/initrd.gz.bak ]; then "
                    "    mv /boot/initrd.gz.bak /boot/initrd.gz; "
                    "  fi; "
                    "fi"
                );
                runTask("/bin/sh", {"-c", initrdCmd});
            }

            if (adminConfig.postInstallTask(PostInstallTask::UpdateGrub) &&
                QFile::exists("/usr/sbin/grub-mkconfig")) {
                socket->write(OUTPUT + QByteArray("\nUpdating GRUB bootloader configuration...") + SEP);
                runTask("/usr/sbin/grub-mkconfig", {"-o", "/boot/grub/grub.cfg"});
            }

            if (adminConfig.postInstallTask(PostInstallTask::UpdateLilo) &&
                QFile::exists("/sbin/lilo") && QFile::exists("/etc/lilo.conf")) {
                socket->write(OUTPUT + QByteArray("\nScanning kernels and updating LILO bootloader...") + SEP);
                const auto liloCmd = QStringLiteral(
                    "LILO_CONF=\"/etc/lilo.conf\"; "
                    "if [ -f \"$LILO_CONF\" ] && [ -x /sbin/lilo ]; then "
                    "  cp -a \"$LILO_CONF\" \"${LILO_CONF}.klass.bak\"; "
                    "  ROOT_DEV=$(grep -E '^[[:space:]]*root[[:space:]]*=' \"$LILO_CONF\" | head -n1 | cut -d= -f2- | tr -d ' '); "
                    "  [ -z \"$ROOT_DEV\" ] && ROOT_DEV=$(mount | grep ' / ' | cut -d' ' -f1); "
                    "  KVER=$(uname -r); "
                    "  if [ ! -e /boot/initrd.gz ] && [ -f \"/boot/initrd-${KVER}.gz\" ]; then "
                    "    ln -sf \"initrd-${KVER}.gz\" /boot/initrd.gz; "
                    "  fi; "
                    "  for k in /boot/vmlinuz-*; do "
                    "    [ -f \"$k\" ] || continue; "
                    "    case \"$k\" in *.bak|*.old|*.tmp) continue ;; esac; "
                    "    if grep -q -F \"$k\" \"$LILO_CONF\"; then "
                    "      continue; "
                    "    fi; "
                    "    KNAME=$(basename \"$k\"); "
                    "    if echo \"$KNAME\" | grep -q \"generic\"; then "
                    "      VER=$(echo \"$KNAME\" | sed 's/vmlinuz-generic-//'); "
                    "      LABEL=\"gen-${VER}\"; "
                    "    elif echo \"$KNAME\" | grep -q \"huge\"; then "
                    "      VER=$(echo \"$KNAME\" | sed 's/vmlinuz-huge-//'); "
                    "      LABEL=\"hug-${VER}\"; "
                    "    else "
                    "      VER=$(echo \"$KNAME\" | sed 's/vmlinuz-//'); "
                    "      LABEL=\"k-${VER}\"; "
                    "    fi; "
                    "    LABEL=$(echo \"$LABEL\" | cut -c1-15); "
                    "    if grep -q -E \"^[[:space:]]*label[[:space:]]*=[[:space:]]*\\\"?${LABEL}\\\"?\" \"$LILO_CONF\"; then "
                    "      LABEL=\"k$(echo \"$LABEL\" | cut -c1-13)\"; "
                    "    fi; "
                    "    INITRD_FILE=\"\"; "
                    "    if [ -f \"/boot/initrd-${VER}.gz\" ]; then "
                    "      INITRD_FILE=\"/boot/initrd-${VER}.gz\"; "
                    "    elif echo \"$KNAME\" | grep -q \"generic\" && [ -f /boot/initrd.gz ]; then "
                    "      INITRD_FILE=\"/boot/initrd.gz\"; "
                    "    fi; "
                    "    echo \"\" >> \"$LILO_CONF\"; "
                    "    echo \"# Added by Klass Package Manager\" >> \"$LILO_CONF\"; "
                    "    echo \"image = $k\" >> \"$LILO_CONF\"; "
                    "    [ -n \"$ROOT_DEV\" ] && echo \"  root = $ROOT_DEV\" >> \"$LILO_CONF\"; "
                    "    echo \"  label = $LABEL\" >> \"$LILO_CONF\"; "
                    "    [ -n \"$INITRD_FILE\" ] && echo \"  initrd = $INITRD_FILE\" >> \"$LILO_CONF\"; "
                    "    echo \"  read-only\" >> \"$LILO_CONF\"; "
                    "  done; "
                    "  if /sbin/lilo -t > /dev/null 2>&1; then "
                    "    /sbin/lilo; "
                    "  else "
                    "    cp -a \"${LILO_CONF}.klass.bak\" \"$LILO_CONF\"; "
                    "    /sbin/lilo; "
                    "  fi; "
                    "fi"
                );
                runTask("/bin/sh", {"-c", liloCmd});
            }

             if (adminConfig.postInstallTask(PostInstallTask::UpdateElilo) &&
                QDir("/boot/efi/EFI/Slackware").exists()) {
                socket->write(OUTPUT + QByteArray("\nUpdating ELILO bootloader (syncing kernels, initrds, and config)...") + SEP);
                const auto eliloCmd = QStringLiteral(
                    "EFI_DIR=\"/boot/efi/EFI/Slackware\"; "
                    "if [ -d \"$EFI_DIR\" ]; then "
                    "  KVER=$(uname -r); "
                    "  if [ ! -e /boot/initrd.gz ] && [ -f \"/boot/initrd-${KVER}.gz\" ]; then "
                    "    ln -sf \"initrd-${KVER}.gz\" /boot/initrd.gz; "
                    "  fi; "
                    "  [ -e /boot/vmlinuz ] && cp -L /boot/vmlinuz \"$EFI_DIR/vmlinuz\"; "
                    "  [ -e /boot/initrd.gz ] && cp -L /boot/initrd.gz \"$EFI_DIR/initrd.gz\"; "
                    "  for k in /boot/vmlinuz-*; do "
                    "    [ -f \"$k\" ] || continue; "
                    "    case \"$k\" in *.bak|*.old|*.tmp) continue ;; esac; "
                    "    cp -L \"$k\" \"$EFI_DIR/$(basename \"$k\")\"; "
                    "  done; "
                    "  for i in /boot/initrd-*.gz; do "
                    "    [ -f \"$i\" ] || continue; "
                    "    case \"$i\" in *.bak|*.old|*.tmp) continue ;; esac; "
                    "    cp -L \"$i\" \"$EFI_DIR/$(basename \"$i\")\"; "
                    "  done; "
                    "  CONF=\"$EFI_DIR/elilo.conf\"; "
                    "  if [ -f \"$CONF\" ]; then "
                    "    cp -a \"$CONF\" \"${CONF}.klass.bak\"; "
                    "    APPEND_LINE=$(grep -E '^[[:space:]]*append[[:space:]]*=' \"$CONF\" | head -n1); "
                    "    ROOT_LINE=$(grep -E '^[[:space:]]*root[[:space:]]*=' \"$CONF\" | head -n1); "
                    "    for k in /boot/vmlinuz-*; do "
                    "      [ -f \"$k\" ] || continue; "
                    "      case \"$k\" in *.bak|*.old|*.tmp) continue ;; esac; "
                    "      KNAME=$(basename \"$k\"); "
                    "      if grep -q -E \"^[[:space:]]*image[[:space:]]*=[[:space:]]*\\\"?${KNAME}\\\"?\" \"$CONF\"; then "
                    "        continue; "
                    "      fi; "
                    "      if echo \"$KNAME\" | grep -q \"generic\"; then "
                    "        VER=$(echo \"$KNAME\" | sed 's/vmlinuz-generic-//'); "
                    "        LABEL=\"gen-${VER}\"; "
                    "      elif echo \"$KNAME\" | grep -q \"huge\"; then "
                    "        VER=$(echo \"$KNAME\" | sed 's/vmlinuz-huge-//'); "
                    "        LABEL=\"hug-${VER}\"; "
                    "      else "
                    "        VER=$(echo \"$KNAME\" | sed 's/vmlinuz-//'); "
                    "        LABEL=\"k-${VER}\"; "
                    "      fi; "
                    "      INITRD_ENTRY=\"\"; "
                    "      if [ -f \"$EFI_DIR/initrd-${VER}.gz\" ]; then "
                    "        INITRD_ENTRY=\"    initrd=initrd-${VER}.gz\"; "
                    "      elif echo \"$KNAME\" | grep -q \"generic\" && [ -f \"$EFI_DIR/initrd.gz\" ]; then "
                    "        INITRD_ENTRY=\"    initrd=initrd.gz\"; "
                    "      fi; "
                    "      echo \"\" >> \"$CONF\"; "
                    "      echo \"# Added by Klass Package Manager\" >> \"$CONF\"; "
                    "      echo \"image=$KNAME\" >> \"$CONF\"; "
                    "      echo \"    label=$LABEL\" >> \"$CONF\"; "
                    "      echo \"    read-only\" >> \"$CONF\"; "
                    "      [ -n \"$ROOT_LINE\" ] && echo \"$ROOT_LINE\" >> \"$CONF\"; "
                    "      [ -n \"$APPEND_LINE\" ] && echo \"$APPEND_LINE\" >> \"$CONF\"; "
                    "      [ -n \"$INITRD_ENTRY\" ] && echo \"$INITRD_ENTRY\" >> \"$CONF\"; "
                    "    done; "
                    "  fi; "
                    "fi"
                );
                runTask("/bin/sh", {"-c", eliloCmd});
            }

            if (adminConfig.postInstallTask(PostInstallTask::ReinstallVBoxModules)) {
                if (QFile::exists("/sbin/vboxconfig")) {
                    socket->write(OUTPUT + QByteArray("\nRebuilding VirtualBox kernel modules (vboxconfig)...") + SEP);
                    runTask("/sbin/vboxconfig", {});
                } else if (QFile::exists("/sbin/rcvboxdrv")) {
                    socket->write(OUTPUT + QByteArray("\nRebuilding VirtualBox kernel modules (rcvboxdrv)...") + SEP);
                    runTask("/sbin/rcvboxdrv", {"setup"});
                } else if (QFile::exists("/etc/rc.d/rc.vboxdrv")) {
                    socket->write(OUTPUT + QByteArray("\nRebuilding VirtualBox kernel modules (rc.vboxdrv)...") + SEP);
                    runTask("/etc/rc.d/rc.vboxdrv", {"setup"});
                } else if (QFile::exists("/etc/init.d/vboxdrv")) {
                    socket->write(OUTPUT + QByteArray("\nRebuilding VirtualBox kernel modules (vboxdrv)...") + SEP);
                    runTask("/etc/init.d/vboxdrv", {"setup"});
                }
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
