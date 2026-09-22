/**
 * @file Mirrors.cpp
 * @brief Class responsible for managing and parsing repository mirror file.
 */

#include <QFile>
#include <QRegularExpression>
#include <QSettings>

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
        if (line.trimmed().startsWith(QLatin1String("OFFICIAL_MIRROR="))) {
            line = targetLine;
            found = true;
            break;
        }
    }

    if (!found) {
        if (const int idx = static_cast<int>(lines.indexOf(QStringLiteral("# Official Repo"))); idx != -1) {
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
    const bool is64 = (detectArch() == "x86_64");
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
    QFile file(DefaultPath().defaultPath("mirrors/valid_mirrors.txt"));
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
    for (QString &line : lines) {
        if (line.trimmed().startsWith(targetKey)) {
            line = activeLine;
            updated = true;
            break;
        }
    }

    if (!updated) {
        if (const int idx = static_cast<int>(lines.indexOf(QStringLiteral("# Third Mirrors"))); idx != -1) {
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

    for (const QString &line : lines) {
        if (line.trimmed().startsWith(targetKey)) {
            removed = true;
            continue;
        }
        updatedLines.append(line);
    }

    return removed ? RepoManager::writeConf(updatedLines) : false;
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
    if (const auto [url, country] = Mirrors::activeOne(); !url.isEmpty())
        repos.append({SLACK_OFICIAL, url});
    repos.append(Mirrors::listActive());

    for (const auto &[name, url]: repos) {
        auto str = "";
        if (url.contains("multilib")) str = "../";
        else if (url.contains("alien")) str = "../../";

        const QUrl remoteUrl(url + (url.endsWith('/') ? "" : "/") + str + "ChangeLog.txt");

        const auto res = Utils::download(remoteUrl, "bytes=0-255");
        if (!res.success) continue;

        const QString remoteFirstLine = QString::fromUtf8(res.data).split('\n').first().trimmed();

        QFile local(KLASS_DATABASE + name + "/ChangeLog.txt");
        if (!local.open(QIODevice::ReadOnly | QIODevice::Text))
            return true;

        const QString localFirstLine = QTextStream(&local).readLine().trimmed();
        local.close();

        if (remoteFirstLine != localFirstLine) {
            Debug::Debug().msg("Update Available in " + name, "RepoManager", {"true"});
            return true;
        }
        Debug::Debug().msg("Update Available in " + name, "RepoManager", {"false"});
    }
    return false;
}
