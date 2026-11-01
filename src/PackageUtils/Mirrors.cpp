/**
 * @file Mirrors.cpp
 * @brief Class responsible for managing and parsing repository mirror file.
 */

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>

#include <Debug.hpp>
#include <DefaultPath.hpp>
#include <Mirrors.hpp>
#include <RepoManager.hpp>
#include <SlackwareDefines.hpp>
#include <Utils.hpp>

/**
 * @brief Parses a line from the mirrors file.
 * @param raw Raw line read from the file.
 * @param outUrl URL extracted from the line.
 * @return True if the line contains a valid URL.
 */
bool Mirrors::parseMirrorLine(const QString &raw, QString &outUrl) {
    QStringView l = QStringView(raw).trimmed();
    if (l.isEmpty())
        return false;

    if (l.startsWith(u'#'))
        l = l.mid(1).trimmed();

    if (!l.contains(u"://") || l.contains(u": ") || l.contains(u"file:") || l.contains(u"cdrom:"))
        return false;

    outUrl = l.toString();
    return true;
}

/**
 * @brief Lists every mirror entry present in the mirrors file for the current version/arch combination.
 * @return Ordered list of MirrorEntry structs.
 */
QList<Mirrors::MirrorEntry> Mirrors::listAll() {
    QFile file(SLACKPKG_MIRRORS);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QString fileContent = QString::fromUtf8(file.readAll());
    file.close();

    QList<MirrorEntry> result;
    auto currentCountry = QStringLiteral("GLOBAL");
    const bool isCurrent = detectChannel() == Channel::Current;
    const QString version = QSysInfo::productVersion();
    static const QRegularExpression re(R"(\(([A-Z]{2})\))");

    for (const auto lines = qTokenize(fileContent, u'\n'); QStringView line: lines) {
        QStringView trimmed = line.trimmed();

        if (trimmed.isEmpty())
            continue;

        if (trimmed.startsWith(u'#') && !trimmed.contains(u"://")) {
            QString commentStr = trimmed.toString();
            if (const auto m = re.match(commentStr); m.hasMatch())
                currentCountry = m.captured(1);
            continue;
        }

        QString rawLine = line.toString();
        QString url;
        if (!parseMirrorLine(rawLine, url))
            continue;

        const bool hasCurrent = url.contains(u"current", Qt::CaseInsensitive);

        if (isCurrent && !hasCurrent)
            continue;

        if (!isCurrent) {
            if (hasCurrent || !url.contains(version, Qt::CaseInsensitive))
                continue;
        }
        result.append(MirrorEntry{std::move(url), currentCountry});
    }
    return result;
}

/**
 * @brief Returns the active mirror, now querying klass.conf.
 * @return The active mirror found in klass.conf or the system fallback.
 */
Mirrors::MirrorEntry Mirrors::activeOne() {
    for (QStringList activeLines = RepoManager::readConf(); const QString &line: activeLines) {
        if (line.startsWith(u"OFFICIAL_MIRROR=")) {
            const QString url = line.mid(16).trimmed();
            return {url, "GLOBAL"};
        }
    }
    return {};
}

/**
 * @brief Sets the active mirror, writing to klass.conf.
 * @param url Full mirror URL including trailing slash.
 * @return True on success, false on I/O error.
 */
bool Mirrors::setActive(const QString &url) {
    QStringList lines = RepoManager::readConf();

    const QString targetLine = QStringLiteral("OFFICIAL_MIRROR=") + url;
    bool found = false;

    for (QString &line: lines) {
        if (line.trimmed().startsWith(u"OFFICIAL_MIRROR=")) {
            line = targetLine;
            found = true;
            break;
        }
    }

    if (!found) {
        if (const int idx = static_cast<int>(lines.indexOf(OFFICIAL_HEADER)); idx != -1) {
            lines.insert(idx + 1, targetLine);
        } else {
            lines.prepend(targetLine);
        }
    }

    return RepoManager::writeConf(lines);
}

/**
 * @brief Validates if a repository URL is compatible with the local system.
 * @param url The fully-resolved repository URL string to evaluate.
 * @return True if the URL matches the machine environment specifications, false otherwise.
 */
bool Mirrors::accept(const QString &url) {
    const QString u = url.toLower();
    const bool hasCurrent = u.contains("current/") || u.contains("dev/");
    const bool is64 = detectArch() == "x86_64";
    const bool has64 = (u.contains("x86_64") || u.contains("slackware64") || u.contains("multilib"))
                       && !u.contains("armv7hl");
    const bool has32 = (u.contains("x86/") || u.contains("i486/") || u.contains("i586/") ||
                        u.contains("i686/")) && !u.contains("slackware64") &&
                       !u.contains("multilib") && !u.contains("armv7hl");

    if (detectChannel() == Channel::Current) {
        if (!hasCurrent)
            return false;

        if (is64)
            return !has32 || has64;

        return !has64;
    }

    if (hasCurrent)
        return false;

    if (is64) {
        if (has32 || !has64)
            return false;
    } else {
        if (has64)
            return false;
    }

    const QStringList parts = QSysInfo::productVersion().split('.');
    if (const QString &systemVer = parts.first(); !u.contains(systemVer))
        return false;

    return true;
}

/**
 * @brief Parses the "Supported Repositories" block from slackpkgplus.conf.
 * @return Compatible repositories with fully-resolved URLs.
 */
QList<Mirrors::MirrorPlusEntry> Mirrors::catalogue() {
    QList<MirrorPlusEntry> result;
    QFile file(DefaultPath::defaultPath("mirrors/valid_mirrors.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        QStringList parts = line.split(QLatin1String(" : "));
        if (parts.size() < 2)
            continue;

        const QString name = parts[0].trimmed();
        const QString url = parts[1].trimmed();

        if (!accept(url))
            continue;

        result.append({name, url});
    }
    return result;
}

/**
 * @brief Returns the currently active THIRDMIRROR entries from klass.conf.
 * @return List of active third-party repositories.
 */
QList<Mirrors::MirrorPlusEntry> Mirrors::listActive() {
    QStringList lines = RepoManager::readConf();
    QList<MirrorPlusEntry> result;

    static const QRegularExpression re(R"(^THIRDMIRROR\['([^']+)'\]\s*=\s*(.+)$)");

    for (const QString &line: lines) {
        if (const QRegularExpressionMatch m = re.match(line.trimmed()); m.hasMatch())
            result.append({m.captured(1), m.captured(2).trimmed()});
    }

    return result;
}

/**
 * @brief Enables or updates a repository entry under the '# Third Mirrors' section.
 * @param entry Repository data to enable or update.
 * @return True on success.
 */
bool Mirrors::enable(const MirrorPlusEntry &entry) {
    QStringList lines = RepoManager::readConf();

    const QString targetKey = QStringLiteral("THIRDMIRROR['") + entry.name + QStringLiteral("']");
    const QString activeLine = targetKey + QLatin1Char('=') + entry.url;

    bool updated = false;
    for (QString &line: lines) {
        if (line.trimmed().startsWith(targetKey)) {
            line = activeLine;
            updated = true;
            break;
        }
    }

    if (!updated) {
        if (const int idx = static_cast<int>(lines.indexOf(THIRD_HEADER)); idx != -1) {
            lines.insert(idx + 1, activeLine);
        } else {
            lines.append(activeLine);
        }
    }

    return RepoManager::writeConf(lines);
}

/**
 * @brief Disables a repository by removing its entry completely from klass.conf.
 * @param name Repository name to remove.
 * @return True on success.
 */
bool Mirrors::disable(const QString &name) {
    const QStringList lines = RepoManager::readConf();

    const QString targetKey = QStringLiteral("THIRDMIRROR['") + name + QStringLiteral("']");
    QStringList updatedLines;
    bool removed = false;

    for (const QString &line: lines) {
        if (line.trimmed().startsWith(targetKey)) {
            removed = true;
            continue;
        }
        updatedLines.append(line);
    }

    if (!removed || !RepoManager::writeConf(updatedLines))
        return false;

    if (QDir repoDir(KLASS_DATABASE + name); repoDir.exists())
        return repoDir.removeRecursively();
    return true;
}

/**
 * @brief Returns the machine architecture as used in mirror paths.
 * @return "x86_64", "x86", or an empty QString when undetermined.
 */
QString Mirrors::detectArch() {
    QString arch = QSysInfo::buildCpuArchitecture();

    if (arch == "i686" || arch == "i586" || arch == "i486" || arch == "i386")
        return "x86";

    return arch;
}

/**
 * @brief Detects whether the system is running -current or a stable release.
 * @return Channel::Current, Channel::Stable or Channel::Unknown.
 */
Mirrors::Channel Mirrors::detectChannel() {
    const QSettings osRelease("/etc/os-release", QSettings::IniFormat);
    const QString value = osRelease.value("VERSION_CODENAME").toString().toLower();

    if (value == "current")
        return Channel::Current;

    if (value == "stable")
        return Channel::Stable;

    return Channel::Unknown;
}

/**
 * @brief Checks online if any active repository has metadata updates available.
 * @return true if any remote repository log is newer than the local cache, false otherwise.
 */
bool Mirrors::isUpdateRequired() {
    QList<MirrorPlusEntry> repos;

    if (const auto [url, country] = activeOne(); !url.isEmpty())
        repos.append({SLACK_OFICIAL, url});
    repos.append(listActive());

    for (const auto &[name, url]: repos) {
        const QStringView modifier = url.contains(u"multilib") ? u"../" : url.contains(u"alien") ? u"../../" : u"";

        QString fullUrl = url;
        if (!fullUrl.endsWith(u'/'))
            fullUrl.append(u'/');
        fullUrl.append(modifier).append(u"ChangeLog.txt");
        const QUrl remoteUrl(fullUrl);

        const auto res = Utils::download(remoteUrl, {}, "bytes=0-255");
        if (!res.success || res.data.isEmpty())
            continue;

        const QString remoteTimestamp = extractTimestamp(QString::fromUtf8(res.data));
        if (remoteTimestamp.isEmpty())
            continue;

        QFile local(KLASS_DATABASE + name + "/ChangeLog.txt");
        if (!local.open(QIODevice::ReadOnly | QIODevice::Text))
            return true;

        const QString localTimestamp = extractTimestamp(QString::fromUtf8(local.read(1024)));
        local.close();

        if (localTimestamp.isEmpty())
            return true;

        if (remoteTimestamp != localTimestamp) {
            Debug::msg("Update Available in " + name, "RepoManager", {remoteTimestamp, DColor::Yellow});
            return true;
        }
        Debug::msg("Repository up to date: " + name, "RepoManager", {localTimestamp, DColor::Green});
    }
    return false;
}

/**
 * @brief Loads the active state for official sub-branches (Pasture, Testing, Extra) from klass.conf.
 * @return An OfficialBranches struct representing the enabled/disabled state of each official sub-branch.
 */
Mirrors::OfficialBranches Mirrors::loadOfficialBranches() {
    OfficialBranches branches;
    for (const QString &line: RepoManager::readConf()) {
        if (const QString trimmed = line.trimmed(); trimmed.startsWith(u"OFFICIAL_PASTURE="))
            branches.pasture = trimmed.mid(17).trimmed().compare(u"true", Qt::CaseInsensitive) == 0;
        else if (trimmed.startsWith(u"OFFICIAL_TESTING="))
            branches.testing = trimmed.mid(17).trimmed().compare(u"true", Qt::CaseInsensitive) == 0;
        else if (trimmed.startsWith(u"OFFICIAL_EXTRA="))
            branches.extra = trimmed.mid(15).trimmed().compare(u"true", Qt::CaseInsensitive) == 0;
    }
    return branches;
}

/**
 * @brief Writes the official branches state under '# Official Repo' in klass.conf and cleans disabled repo directories.
 * @param branches An OfficialBranches struct containing the updated state for each official branch.
 * @return True if the configuration was successfully written to disk; false on I/O failure.
 */
bool Mirrors::setOfficialBranches(const OfficialBranches &branches) {
    QStringList lines = RepoManager::readConf();

    auto updateOrInsert = [&](const QString &key, const bool val) {
        const QString targetLine = key + u'=' + (val ? QStringLiteral("true") : QStringLiteral("false"));
        bool updated = false;
        for (QString &line: lines) {
            if (line.trimmed().startsWith(key + u'=')) {
                line = targetLine;
                updated = true;
                break;
            }
        }
        if (!updated) {
            if (const int idx = static_cast<int>(lines.indexOf(OFFICIAL_HEADER)); idx != -1)
                lines.insert(idx + 1, targetLine);
            else
                lines.prepend(targetLine);
        }
    };

    updateOrInsert(QStringLiteral("OFFICIAL_PASTURE"), branches.pasture);
    updateOrInsert(QStringLiteral("OFFICIAL_TESTING"), branches.testing);
    updateOrInsert(QStringLiteral("OFFICIAL_EXTRA"), branches.extra);

    if (!branches.pasture) {
        if (QDir dir(KLASS_DATABASE + SLACK_PASTURE); dir.exists())
            dir.removeRecursively();
    }
    if (!branches.testing) {
        if (QDir dir(KLASS_DATABASE + SLACK_TESTING); dir.exists())
            dir.removeRecursively();
    }
    if (!branches.extra) {
        if (QDir dir(KLASS_DATABASE + SLACK_EXTRA); dir.exists())
            dir.removeRecursively();
    }

    return RepoManager::writeConf(lines);
}

/**
 * @brief Extracts the first non-decorative, non-empty timestamp line from a ChangeLog buffer.
 * @details Skips decorative frame lines (such as '+---' or '===') and empty lines to obtain the real date entry.
 * @param text Raw ChangeLog content.
 * @return The trimmed timestamp line, or an empty QString if none found.
 */
QString Mirrors::extractTimestamp(const QString &text) {
    for (const auto lines = qTokenize(text, u'\n'); QStringView line: lines) {
        const QStringView trimmed = line.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith(u'+') && !trimmed.startsWith(u'='))
            return trimmed.toString();
    }
    return {};
}
