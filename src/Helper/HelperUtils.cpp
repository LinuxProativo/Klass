/**
 * @file HelperUtils.cpp
 * Implementation of helper utilities for network operations and database updates.
 */

#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QProcess>
#include <QRegularExpression>

#include <AdminConfigManager.hpp>
#include <HelperUtils.hpp>
#include <Mirrors.hpp>
#include <RepoVerificationManager.hpp>
#include <SlackwareDefines.hpp>
#include <Utils.hpp>

#include <gpgme.h>
#include <sys/utsname.h>

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

        const bool isOfficialRepo = name == SLACK_OFICIAL || name == SLACK_PATCHES || name == SLACK_EXTRA ||
                                    name == SLACK_TESTING || name == SLACK_PASTURE;

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

            for (const QString &targetFile: targets) {
                const QString targetUrl = url + (url.endsWith('/') ? "" : "/") + targetFile;
                const auto saveName = fileName.startsWith("ChangeLog") ? QStringLiteral("ChangeLog.txt") : targetFile;
                const QString savePath = KLASS_DATABASE + name + "/" + saveName;

                const auto res = Utils::download(QUrl(targetUrl), savePath, {}, [this, socket] {
                    return checkSocketCancellation(socket, operationCancelled);
                });

                if (operationCancelled) {
                    QFile::remove(savePath);
                    break;
                }

                if (res.success) {
                    socket->write(OUTPUT + QByteArray("    --> Download File ") + targetUrl.toUtf8() + SEP);
                    break;
                }
                const auto msg = "StatusCode " + QString::number(res.statusCode) + " -> " + QString::number(res.error);
                QTextStream(stdout) << msg;
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
                    OUTPUT + QByteArray("ERROR: Missing CHECKSUMS or signature (.asc) file for mirror ")
                    + name.toUtf8() + ". Purging directory and aborting update." + SEP +
                    QByteArray("UPDATE_DONE") + SEP);
                QDir(dbDir).removeRecursively();
                operationCancelled = true;
                break;
            }

            socket->write(OUTPUT + QByteArray("    --> Verifying CHECKSUMS GPG signature...") + SEP);
            if (!verifyGpgSignature(checksumAscFile, checksumFile, name)) {
                socket->write(OUTPUT + QByteArray("CHECKSUMS signature verification FAILED for mirror ")
                              + name.toUtf8() + ". Purging corrupted directory and aborting update." + SEP +
                              QByteArray("UPDATE_DONE") + SEP);
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
            const auto res = Utils::download(QUrl(pkg.url), savePath, {}, [this, socket] {
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
                if (res.computedMd5 != pkg.md5) {
                    socket->write(
                        OUTPUT + QByteArray("ERROR: Failed to verify checksum for ") + fileName.toUtf8() + ". Aborting."
                        + SEP + QByteArray("TRANSACTION_DONE") + SEP);
                    QFile::remove(savePath);
                    operationCancelled = true;
                    break;
                }
                socket->write(OUTPUT + QByteArray("    --> Checksum successfully validated.") + SEP);
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
                const auto ascRes = Utils::download(QUrl(pkg.ascUrl), ascPath, {}, [this, socket] {
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
                    if (ascRes.computedMd5 != pkg.ascMd5) {
                        socket->write(
                            OUTPUT + QByteArray("ERROR: MD5 mismatch for signature file ") + ascName.toUtf8() +
                            ". Aborting." + SEP + QByteArray("TRANSACTION_DONE") + SEP);
                        QFile::remove(ascPath);
                        operationCancelled = true;
                        break;
                    }
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

    if (!installList.isEmpty() || !reinstallList.isEmpty()) {
        if (!adminConfig.keepCache()) {
            socket->write(OUTPUT + QByteArray("\nCleaning downloaded packages cache...") + SEP);
            if (QDir cacheDir(KLASS_PACKAGES); cacheDir.exists()) {
                cacheDir.removeRecursively();
                (void) QDir().mkpath(KLASS_PACKAGES);
            }
            socket->write(OUTPUT + QByteArray("    --> Package cache cleaned.\n") + SEP);
        }
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
                socket->write(OUTPUT + QByteArray("\nGenerating initrd for generic kernel...") + SEP);

                const auto initrdPath = QStringLiteral("/boot/initrd.gz");
                const auto backupPath = QStringLiteral("/boot/initrd.gz.bak");

                if (QFile::exists(initrdPath)) {
                    QFile::remove(backupPath);
                    QFile::copy(initrdPath, backupPath);
                }

                runTask("/bin/sh", {"-c", "/usr/share/mkinitrd/mkinitrd_command_generator.sh | bash"});

                if (QFile::exists(initrdPath)) {
                    utsname u{};
                    uname(&u);
                    const QString kver = QString::fromLatin1(u.release);
                    const QString versionedName = QStringLiteral("initrd-%1.gz").arg(kver);
                    const QString versionedPath = QStringLiteral("/boot/") + versionedName;

                    QFile::remove(versionedPath);
                    if (QFile::rename(initrdPath, versionedPath)) {
                        QFile::remove(initrdPath);
                        QFile::link(versionedName, initrdPath);
                        QFile::remove(backupPath);
                        socket->write(
                            OUTPUT + QByteArray("    --> Created /boot/") + versionedName.toUtf8() +
                            " and updated /boot/initrd.gz\n" + SEP);
                    }
                } else if (QFile::exists(backupPath)) {
                    QFile::rename(backupPath, initrdPath);
                    socket->write(OUTPUT + QByteArray("Failed to generate new initrd. Restored backup.\n") + SEP);
                }
            }

            if (adminConfig.postInstallTask(PostInstallTask::UpdateGrub) &&
                QFile::exists("/usr/sbin/grub-mkconfig")) {
                socket->write(OUTPUT + QByteArray("\nUpdating GRUB bootloader configuration...") + SEP);
                runTask("/usr/sbin/grub-mkconfig", {"-o", "/boot/grub/grub.cfg"});
            }

            if (adminConfig.postInstallTask(PostInstallTask::UpdateLilo) &&
                QFile::exists("/sbin/lilo") && QFile::exists("/etc/lilo.conf")) {
                socket->write(OUTPUT + QByteArray("\nScanning kernels and updating LILO bootloader...") + SEP);

                const auto liloConfPath = QStringLiteral("/etc/lilo.conf");
                const auto backupPath = QStringLiteral("/etc/lilo.conf.klass.bak");

                QFile::remove(backupPath);
                QFile::copy(liloConfPath, backupPath);

                QString liloContent;
                if (QFile f(liloConfPath); f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    liloContent = QString::fromUtf8(f.readAll());
                    f.close();
                }

                QString rootDev;
                static const QRegularExpression rootRegex(R"(^[ \t]*root[ \t]*=[ \t]*([^\s\r\n]+))",
                                                          QRegularExpression::MultilineOption);
                if (const auto match = rootRegex.match(liloContent); match.hasMatch())
                    rootDev = match.captured(1).trimmed();

                utsname u{};
                uname(&u);
                const QString kver = QString::fromLatin1(u.release);
                const QString versionedInitrd = QStringLiteral("/boot/initrd-%1.gz").arg(kver);
                if (!QFile::exists("/boot/initrd.gz") && QFile::exists(versionedInitrd))
                    QFile::link(QStringLiteral("initrd-%1.gz").arg(kver), QStringLiteral("/boot/initrd.gz"));

                const QDir bootDir(QStringLiteral("/boot"));
                const QStringList kernelFiles = bootDir.entryList({QStringLiteral("vmlinuz-*")}, QDir::Files,
                                                                  QDir::Name);

                QString newEntries;
                for (const QString &kName: kernelFiles) {
                    if (kName.endsWith(QLatin1String(".bak")) || kName.endsWith(QLatin1String(".old")) ||
                        kName.endsWith(QLatin1String(".tmp")))
                        continue;

                    const QString fullKernelPath = QStringLiteral("/boot/") + kName;

                    if (liloContent.contains(fullKernelPath))
                        continue;

                    QString ver;
                    QString label;
                    if (kName.startsWith(QLatin1String("vmlinuz-generic-"))) {
                        ver = kName.mid(16);
                        label = QStringLiteral("gen-") + ver;
                    } else if (kName.startsWith(QLatin1String("vmlinuz-huge-"))) {
                        ver = kName.mid(13);
                        label = QStringLiteral("hug-") + ver;
                    } else if (kName.startsWith(QLatin1String("vmlinuz-"))) {
                        ver = kName.mid(8);
                        label = QStringLiteral("k-") + ver;
                    } else {
                        continue;
                    }

                    if (label.length() > 15)
                        label = label.left(15);

                    const QRegularExpression labelRegex(
                        QStringLiteral(R"(^[ \t]*label[ \t]*=[ \t]*"?%1"?)").arg(QRegularExpression::escape(label)),
                        QRegularExpression::MultilineOption);
                    if (liloContent.contains(labelRegex) || newEntries.contains(labelRegex)) {
                        label = QStringLiteral("k") + label.left(13);
                    }

                    QString initrdPath;
                    if (QFile::exists(QStringLiteral("/boot/initrd-%1.gz").arg(ver))) {
                        initrdPath = QStringLiteral("/boot/initrd-%1.gz").arg(ver);
                    } else if (kName.contains(QLatin1String("generic")) && QFile::exists("/boot/initrd.gz")) {
                        initrdPath = QStringLiteral("/boot/initrd.gz");
                    }

                    newEntries += QStringLiteral("\n# Added by Klass Package Manager\n");
                    newEntries += QStringLiteral("image = %1\n").arg(fullKernelPath);
                    if (!rootDev.isEmpty())
                        newEntries += QStringLiteral("  root = %1\n").arg(rootDev);
                    newEntries += QStringLiteral("  label = %1\n").arg(label);
                    if (!initrdPath.isEmpty())
                        newEntries += QStringLiteral("  initrd = %1\n").arg(initrdPath);
                    newEntries += QStringLiteral("  read-only\n");
                }

                if (!newEntries.isEmpty()) {
                    if (QFile f(liloConfPath); f.open(QIODevice::Append | QIODevice::Text)) {
                        QTextStream out(&f);
                        out << newEntries;
                        f.close();
                    }
                }

                if (QProcess::execute(QStringLiteral("/sbin/lilo"), {QStringLiteral("-t")}) != 0) {
                    socket->write(
                        OUTPUT + QByteArray(
                            "LILO test failed with new entries. Restoring previous lilo.conf.\n") + SEP);
                    QFile::remove(liloConfPath);
                    QFile::copy(backupPath, liloConfPath);
                }

                runTask(QStringLiteral("/sbin/lilo"), {});
                QFile::remove(backupPath);
            }

            if (adminConfig.postInstallTask(PostInstallTask::UpdateElilo)) {
                if (const auto efiDir = QStringLiteral("/boot/efi/EFI/Slackware"); QDir(efiDir).exists()) {
                    socket->write(
                        OUTPUT + QByteArray("\nUpdating ELILO bootloader (kernels, initrds and config)...") + SEP);

                    utsname u{};
                    uname(&u);
                    const QString kver = QString::fromLatin1(u.release);
                    const QString versionedInitrd = QStringLiteral("/boot/initrd-%1.gz").arg(kver);
                    if (!QFile::exists("/boot/initrd.gz") && QFile::exists(versionedInitrd)) {
                        QFile::link(QStringLiteral("initrd-%1.gz").arg(kver), QStringLiteral("/boot/initrd.gz"));
                    }

                    auto safeCopy = [](const QString &src, const QString &dst) {
                        if (QFile::exists(src)) {
                            QFile::remove(dst);
                            QFile::copy(src, dst);
                        }
                    };

                    safeCopy(QStringLiteral("/boot/vmlinuz"), efiDir + QStringLiteral("/vmlinuz"));
                    safeCopy(QStringLiteral("/boot/initrd.gz"), efiDir + QStringLiteral("/initrd.gz"));

                    const QDir bootDir(QStringLiteral("/boot"));
                    const QStringList kernelFiles = bootDir.entryList({QStringLiteral("vmlinuz-*")}, QDir::Files,
                                                                      QDir::Name);
                    for (const QString &kName: kernelFiles) {
                        if (kName.endsWith(QLatin1String(".bak")) || kName.endsWith(QLatin1String(".old")) || kName.
                            endsWith(QLatin1String(".tmp")))
                            continue;
                        safeCopy(QStringLiteral("/boot/") + kName, efiDir + u'/' + kName);
                    }

                    const QStringList initrdFiles = bootDir.entryList({QStringLiteral("initrd-*.gz")}, QDir::Files,
                                                                      QDir::Name);
                    for (const QString &iName: initrdFiles) {
                        if (iName.endsWith(QLatin1String(".bak")) || iName.endsWith(QLatin1String(".old")) || iName.
                            endsWith(QLatin1String(".tmp")))
                            continue;
                        safeCopy(QStringLiteral("/boot/") + iName, efiDir + u'/' + iName);
                    }

                    if (const QString confPath = efiDir + QStringLiteral("/elilo.conf"); QFile::exists(confPath)) {
                        const QString confBackup = confPath + QStringLiteral(".klass.bak");
                        QFile::remove(confBackup);
                        QFile::copy(confPath, confBackup);

                        QString eliloContent;
                        if (QFile f(confPath); f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            eliloContent = QString::fromUtf8(f.readAll());
                            f.close();
                        }

                        QString appendLine;
                        static const QRegularExpression appendRegex(R"(^[ \t]*append[ \t]*=.*$)",
                                                                    QRegularExpression::MultilineOption);
                        if (const auto match = appendRegex.match(eliloContent); match.hasMatch())
                            appendLine = match.captured(0).trimmed();

                        QString rootLine;
                        static const QRegularExpression rootRegex(R"(^[ \t]*root[ \t]*=.*$)",
                                                                  QRegularExpression::MultilineOption);
                        if (const auto match = rootRegex.match(eliloContent); match.hasMatch())
                            rootLine = match.captured(0).trimmed();

                        QString newEntries;
                        for (const QString &kName: kernelFiles) {
                            if (kName.endsWith(QLatin1String(".bak")) || kName.endsWith(QLatin1String(".old")) ||
                                kName.endsWith(QLatin1String(".tmp")))
                                continue;

                            const QRegularExpression imageRegex(
                                QStringLiteral(R"(^[ \t]*image[ \t]*=[ \t]*"?%1"?)").arg(
                                    QRegularExpression::escape(kName)), QRegularExpression::MultilineOption);
                            if (eliloContent.contains(imageRegex) || newEntries.contains(imageRegex))
                                continue;

                            QString ver, label;
                            if (kName.startsWith(QLatin1String("vmlinuz-generic-"))) {
                                ver = kName.mid(16);
                                label = QStringLiteral("gen-") + ver;
                            } else if (kName.startsWith(QLatin1String("vmlinuz-huge-"))) {
                                ver = kName.mid(13);
                                label = QStringLiteral("hug-") + ver;
                            } else if (kName.startsWith(QLatin1String("vmlinuz-"))) {
                                ver = kName.mid(8);
                                label = QStringLiteral("k-") + ver;
                            } else {
                                continue;
                            }

                            QString initrdEntry;
                            if (QFile::exists(efiDir + QStringLiteral("/initrd-%1.gz").arg(ver))) {
                                initrdEntry = QStringLiteral("    initrd=initrd-%1.gz\n").arg(ver);
                            } else if (kName.contains(QLatin1String("generic")) &&
                                       QFile::exists(efiDir + QStringLiteral("/initrd.gz"))) {
                                initrdEntry = QStringLiteral("    initrd=initrd.gz\n");
                            }

                            newEntries += QStringLiteral("\n# Added by Klass Package Manager\n");
                            newEntries += QStringLiteral("image=%1\n").arg(kName);
                            newEntries += QStringLiteral("    label=%1\n").arg(label);
                            newEntries += QStringLiteral("    read-only\n");
                            if (!rootLine.isEmpty())
                                newEntries += QStringLiteral("    %1\n").arg(rootLine);
                            if (!appendLine.isEmpty())
                                newEntries += QStringLiteral("    %1\n").arg(appendLine);
                            if (!initrdEntry.isEmpty())
                                newEntries += initrdEntry;
                        }

                        if (!newEntries.isEmpty()) {
                            if (QFile f(confPath); f.open(QIODevice::Append | QIODevice::Text)) {
                                QTextStream out(&f);
                                out << newEntries;
                                f.close();
                            }
                        }
                        QFile::remove(confBackup);
                    }

                    socket->write(
                        OUTPUT + QByteArray("    --> ELILO EFI files and config successfully synchronized.\n") + SEP);
                }
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
