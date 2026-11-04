/**
 * @file Helper.cpp
 * @brief Manages privileged execution requests and communication between the program and helper.
 */

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QNetworkReply>
#include <QTextStream>

#include <AdminConfigManager.hpp>
#include <Helper.hpp>
#include <Mirrors.hpp>
#include <Packages.hpp>
#include <RepoVerificationManager.hpp>
#include <RulesManager.hpp>
#include <SlackwareDefines.hpp>
#include <TaskManager.hpp>

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define WRITE(ok, ok_msg, err_msg) ((ok ? QByteArray(LOG) + ok_msg : QByteArray(ERROR) + err_msg) + SEP)

/**
 * @brief Constructs the helper service and initializes the local socket server.
 * @param parent Parent object.
 */
Helper::Helper(QObject *parent) : QObject(parent) {
    helperUtils = new HelperUtils(this);
    server = new QLocalServer(this);
    server->setSocketOptions(QLocalServer::WorldAccessOption | QLocalServer::AbstractNamespaceOption);

    if (!server->listen(SOCKET_PATH))
        QTextStream(stdout) << server->errorString();

    connect(server, &QLocalServer::newConnection, this, [this] {
        if (clientSocket) {
            auto *socket = server->nextPendingConnection();
            socket->disconnectFromServer();
            socket->deleteLater();
            QTextStream(stdout) << "Rejected second connection";
            return;
        }

        clientSocket = server->nextPendingConnection();

        if (!validateClient(clientSocket)) {
            clientSocket->disconnectFromServer();
            clientSocket->deleteLater();
            clientSocket = nullptr;
            return;
        }

        auto *task = new TaskManager(clientSocket, clientSocket);

        connect(clientSocket, &QLocalSocket::readyRead, this, [task, this] {
            for (const QByteArray data = clientSocket->readAll(); const QByteArray &rawCmd: data.split(SEP)) {
                const QByteArray raw = rawCmd.trimmed();
                if (raw.isEmpty()) continue;

                if (raw == "CANCEL") {
                    task->cancelCommand();
                } else if (raw == "QUIT") {
                    QCoreApplication::quit();
                } else if (raw == "CANCEL_OPERATION") {
                    helperUtils->setOperationCancelled(true);
                } else if (raw == "UPDATE") {
                    helperUtils->setOperationCancelled(false);
                    helperUtils->performUpdate(clientSocket);
                } else if (raw.startsWith("SETMIRROR:")) {
                    if (const QString url = QString::fromUtf8(raw.mid(10)).trimmed(); !url.isEmpty()) {
                        const bool ok = Mirrors::setActive(url);
                        clientSocket->write(WRITE(ok, "Mirror updated", "Failed to update mirror"));
                    }
                } else if (raw.startsWith("ENABLEREPOS:")) {
                    for (const auto p = QString::fromUtf8(raw.mid(12)).trimmed().split(';', Qt::SkipEmptyParts);
                         const QString &part: p) {
                        const int sep = static_cast<int>(part.indexOf('='));
                        if (sep <= 0)
                            continue;
                        const Mirrors::MirrorPlusEntry entry{part.left(sep), part.mid(sep + 1)};
                        const bool ok = Mirrors::enable(entry);
                        clientSocket->write(WRITE(ok, "Repo enabled", "Failed to enable repo"));
                    }
                } else if (raw.startsWith("SETOFFICIALBRANCHES:")) {
                    const QString payload = QString::fromUtf8(raw.mid(20)).trimmed();
                    Mirrors::OfficialBranches branches;
                    for (const auto &part: payload.split(';', Qt::SkipEmptyParts)) {
                        const QStringList kv = part.split('=');
                        if (kv.size() != 2) continue;
                        const bool v = (kv[1].trimmed().compare(QLatin1String("true"), Qt::CaseInsensitive) == 0 || kv[1].trimmed() == QLatin1String("1"));
                        if (kv[0] == QLatin1String("pasture")) branches.pasture = v;
                        else if (kv[0] == QLatin1String("testing")) branches.testing = v;
                        else if (kv[0] == QLatin1String("extra")) branches.extra = v;
                    }
                    const bool ok = Mirrors::setOfficialBranches(branches);
                    clientSocket->write(WRITE(ok, "Official branches updated", "Failed to update official branches"));
                } else if (raw.startsWith("SETREPOVERIFICATION:")) {
                    const QString payload = QString::fromUtf8(raw.mid(20)).trimmed();
                    if (const QStringList parts = payload.split(','); parts.size() == 5) {
                        RepoVerificationEntry entry;
                        entry.repo = parts[0];
                        entry.verifySig = parts[1] == QLatin1String("true") || parts[1] == QLatin1String("1");
                        entry.verifyChecksumSig = parts[2] == QLatin1String("true") || parts[2] == QLatin1String("1");
                        entry.verifyChecksum = parts[3] == QLatin1String("true") || parts[3] == QLatin1String("1");
                        entry.preferredChecksum = parts[4];
                        const bool ok = RepoVerificationManager::saveVerification(entry);
                        clientSocket->write(WRITE(ok, "Repo verification updated", "Failed to update repo verification"));
                    }
                } else if (raw.startsWith("DISABLEREPOS:")) {
                    for (const auto names = QString::fromUtf8(raw.mid(13)).trimmed().split(';', Qt::SkipEmptyParts);
                         const QString &name: names) {
                        RepoVerificationManager::removeVerification(name);
                        const bool ok = Mirrors::disable(name);
                        clientSocket->write(WRITE(ok, "Repo disabled", "Failed to disable repo"));
                    }
                } else if (raw.startsWith("ADDRULE:")) {
                    const QString payload = QString::fromUtf8(raw.mid(8)).trimmed();
                    if (const QStringList parts = payload.split(','); parts.size() == 4) {
                        const RuleEntry rule{parts[0], parts[1], parts[2], parts[3]};
                        const bool ok = RulesManager::addRule(rule);
                        clientSocket->write(WRITE(ok, "Rule added", "No rule added") + "UPDATE_RULES" + SEP);
                    }
                } else if (raw.startsWith("DELRULES:")) {
                    const QString payload = QString::fromUtf8(raw.mid(9)).trimmed();
                    QList<RuleEntry> rulesToToRemove;
                    for (const auto rules = payload.split(';', Qt::SkipEmptyParts); const QString &block: rules) {
                        if (const QStringList parts = block.split(','); parts.size() == 4)
                            rulesToToRemove.append(RuleEntry{parts[0], parts[1], parts[2], parts[3]});
                    }
                    const bool ok = RulesManager::removeRules(rulesToToRemove);
                    clientSocket->write(WRITE(ok, "Rules removed", "No rules removed") + "UPDATE_RULES" + SEP);
                } else if (raw.startsWith("EDITRULE:")) {
                    const QString payload = QString::fromUtf8(raw.mid(9)).trimmed();
                    if (const QStringList tuples = payload.split('|'); tuples.size() == 2) {
                        const QStringList oldParts = tuples[0].split(',');
                        const QStringList newParts = tuples[1].split(',');
                        if (oldParts.size() == 4 && newParts.size() == 4) {
                            const RuleEntry oldRule{oldParts[0], oldParts[1], oldParts[2], oldParts[3]};
                            const RuleEntry newRule{newParts[0], newParts[1], newParts[2], newParts[3]};
                            const bool ok = RulesManager::editRule(oldRule, newRule);
                            clientSocket->write(WRITE(ok, "Rule edited", "No rule edited") + "UPDATE_RULES" + SEP);
                        }
                    }
                } else if (raw.startsWith("TRANSACTION:")) {
                    const QString payload = QString::fromUtf8(raw.mid(12));
                    QList<TransactionPkg> transactionList;

                    for (const QString &actionBlock: payload.split(';', Qt::SkipEmptyParts)) {
                        const int eqIdx = static_cast<int>(actionBlock.indexOf('='));
                        if (eqIdx == -1) continue;

                        const QString actionType = actionBlock.left(eqIdx);
                        const QStringList pkgs = actionBlock.mid(eqIdx + 1).split(',', Qt::SkipEmptyParts);
                        for (const QString &pkgStr: pkgs) {
                            if (const QStringList parts = pkgStr.split('|'); parts.size() >= 8) {
                                TransactionPkg tp;
                                tp.action = actionType;
                                tp.name = parts[0];
                                tp.version = parts[1];
                                tp.repo = parts[2];
                                tp.category = parts[3];
                                tp.url = parts[4];
                                tp.ascUrl = parts[5];
                                tp.md5 = parts[6];
                                tp.ascMd5 = parts[7];

                                transactionList.append(tp);
                            }
                        }
                    }

                    helperUtils->setOperationCancelled(false);
                    helperUtils->processTransaction(clientSocket, task, transactionList);
                } else if (raw.startsWith("SETADMINCONFIG:")) {
                    const QString payload = QString::fromUtf8(raw.mid(15)).trimmed();
                    const bool ok = AdminConfigManager::savePayload(payload);
                    clientSocket->write(WRITE(ok, "Admin config saved", "Failed to save admin config") + SEP);
                } else if (raw.startsWith(INPUT_CHAR)) {
                    task->sendInput(raw.mid(INPUT_SIZE));
                }
            }
        });

        connect(clientSocket, &QLocalSocket::disconnected, qApp, [this] {
            clientSocket->deleteLater();
            clientSocket = nullptr;
            QCoreApplication::quit();
        });
    });
}

/**
 * Destructor for Helper
 */
Helper::~Helper() {
    if (helperUtils)
        helperUtils->setOperationCancelled(true);

    if (clientSocket) {
        clientSocket->disconnectFromServer();
        clientSocket->abort();
    }

    if (server)
        server->close();
}

/**
 * @brief Validates whether the connected client is authorized.
 * @param socket - Client socket.
 * @return bool - True if the client is trusted.
 */
bool Helper::validateClient(const QLocalSocket *socket) {
    if (!socket)
        return false;

    const qintptr fd = socket->socketDescriptor();

    if (fd < 0)
        return false;

    ucred cred{};
    socklen_t len = sizeof(cred);

    if (getsockopt(static_cast<int>(fd), SOL_SOCKET, SO_PEERCRED, &cred, &len) != 0) {
        QTextStream(stdout) << "SO_PEERCRED failed: " << strerror(errno);
        return false;
    }

    if (cred.pid <= 0 || cred.uid == 0 || geteuid() != 0) {
        QTextStream(stdout) << "Client or Helper rejected";
        return false;
    }

    const QString exe = QFileInfo(QString("/proc/%1/exe").arg(cred.pid)).canonicalFilePath();
    if (exe != QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath()) {
        QTextStream(stdout) << "Rejected executable: " << exe;
        return false;
    }

    return true;
}
