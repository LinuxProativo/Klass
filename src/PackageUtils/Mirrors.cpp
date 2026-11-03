/**
 * @file Mirrors.cpp
 * @brief Class responsible for managing and parsing repository mirror files and URLs.
 */

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>

#include <Debug.hpp>
#include <DefaultPath.hpp>
#include <Mirrors.hpp>
#include <RepoUtils.hpp>
#include <SlackwareDefines.hpp>
#include <Utils.hpp>

/**
 * @brief Appends appropriate branch path modifiers to the official base URL.
 * @param baseUrl Official base repository URL.
 * @param branchName Branch identifier (e.g. SLACK_PATCHES, SLACK_EXTRA, etc.).
 * @return Resolved URL with trailing slash.
 */
QString Mirrors::branchUrl(const QString &baseUrl, const QString &branchName) {
    if (branchName == SLACK_OFICIAL)
        return baseUrl;

    QString base = baseUrl.endsWith(u'/') ? baseUrl : baseUrl + u'/';

    if (branchName == SLACK_PATCHES) return base + QStringLiteral("patches/");
    if (branchName == SLACK_EXTRA) return base + QStringLiteral("extra/");
    if (branchName == SLACK_TESTING) return base + QStringLiteral("testing/");
    if (branchName == SLACK_PASTURE) return base + QStringLiteral("pasture/");

    return baseUrl;
}

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

    const QString fileContent = QString::fromUtf8(file.readAll());
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
            const QString commentStr = trimmed.toString();
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
 * @brief Returns the active mirror querying klass.conf.
 * @return The active mirror found in klass.conf or an empty entry.
 */
Mirrors::MirrorEntry Mirrors::activeOne() {
    const QString url = RepoUtils::readConfigValue(QStringLiteral("OFFICIAL_MIRROR="));
    if (!url.isEmpty())
        return {url, QStringLiteral("GLOBAL")};
    return {};
}

/**
 * @brief Sets the active mirror, writing to klass.conf.
 * @param url Full mirror URL including trailing slash.
 * @return True on success, false on I/O error.
 */
bool Mirrors::setActive(const QString &url) {
    const QString targetLine = QStringLiteral("OFFICIAL_MIRROR=") + url;
    return RepoUtils::setConfigValue(OFFICIAL_HEADER, QStringLiteral("OFFICIAL_MIRROR="), targetLine);
}

/**
 * @brief Validates if a repository URL is compatible with the local system.
 * @param url The fully-resolved repository URL string to evaluate.
 * @return True if the URL matches the machine environment specifications, false otherwise.
 */
bool Mirrors::accept(const QString &url) {
    const QString u = url.toLower();
    const bool hasCurrent = u.contains(QStringLiteral("current/")) || u.contains(QStringLiteral("dev/"));
    const bool is64 = RepoUtils::detectArch() == QStringLiteral("x86_64");
    const bool has64 = (u.contains(QStringLiteral("x86_64")) || u.contains(QStringLiteral("slackware64")) ||
                        u.contains(QStringLiteral("multilib"))) && !u.contains(QStringLiteral("armv7hl")) &&
                       !u.contains(QStringLiteral("aarch64"));
    const bool has32 = (u.contains(QStringLiteral("x86/")) || u.contains(QStringLiteral("i486/")) ||
                        u.contains(QStringLiteral("i586/")) || u.contains(QStringLiteral("i686/"))) &&
                       !u.contains(QStringLiteral("slackware64")) && !u.contains(QStringLiteral("multilib")) &&
                       !u.contains(QStringLiteral("armv7hl")) && !u.contains(QStringLiteral("aarch64"));

    if (detectChannel() == Channel::Current) {
        if (!hasCurrent)
            return false;

        if (is64)
            return !has32 && has64;

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

    const QStringList parts = QSysInfo::productVersion().split(u'.');
    if (const QString &systemVer = parts.first(); !u.contains(systemVer))
        return false;

    return true;
}

/**
 * @brief Parses valid third-party mirrors cataloged in the local directory.
 * @return Compatible repositories with fully-resolved URLs.
 */
QList<Mirrors::MirrorPlusEntry> Mirrors::catalogue() {
    QList<MirrorPlusEntry> result;
    QFile file(DefaultPath::defaultPath("mirrors/valid_mirrors.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        const QStringList parts = line.split(QLatin1String(" : "));
        if (parts.size() < 2)
            continue;

        const QString name = parts[0].trimmed();
        const QString url = parts[1].trimmed();

        if (accept(url))
            result.append({name, url});
    }
    return result;
}

/**
 * @brief Returns the currently active THIRDMIRROR entries from klass.conf.
 * @return List of active third-party repositories.
 */
QList<Mirrors::MirrorPlusEntry> Mirrors::listActive() {
    const QStringList lines = RepoUtils::readConf();
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
    const QString targetKey = QStringLiteral("THIRDMIRROR['") + entry.name + QStringLiteral("']");
    const QString activeLine = targetKey + QLatin1Char('=') + entry.url;
    return RepoUtils::setConfigValue(THIRD_HEADER, targetKey, activeLine);
}

/**
 * @brief Disables a repository by removing its entry completely from klass.conf and removing its cached files.
 * @param name Repository name to remove.
 * @return True on success.
 */
bool Mirrors::disable(const QString &name) {
    const QString targetKey = QStringLiteral("THIRDMIRROR['") + name + QStringLiteral("']");
    if (!RepoUtils::removeConfigValue(targetKey))
        return false;

    if (QDir repoDir(KLASS_DATABASE + name); repoDir.exists())
        return repoDir.removeRecursively();

    return true;
}

/**
 * @brief Detects whether the system is running -current or a stable release.
 * @return Channel::Current, Channel::Stable or Channel::Unknown.
 */
Mirrors::Channel Mirrors::detectChannel() {
    const QSettings osRelease(QStringLiteral("/etc/os-release"), QSettings::IniFormat);
    const QString value = osRelease.value(QStringLiteral("VERSION_CODENAME")).toString().toLower();

    if (value == QStringLiteral("current"))
        return Channel::Current;

    return value == QStringLiteral("stable") ? Channel::Stable : Channel::Unknown;
}

/**
 * @brief Checks online if any active repository has metadata updates available.
 * @return True if any remote repository log is newer than the local cache, false otherwise.
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
        fullUrl.append(modifier).append(QStringLiteral("ChangeLog.txt"));

        const auto res = Utils::download(QUrl(fullUrl), {}, "bytes=0-255");
        if (!res.success || res.data.isEmpty())
            continue;

        const QString remoteTimestamp = extractTimestamp(QString::fromUtf8(res.data));
        if (remoteTimestamp.isEmpty())
            continue;

        QFile local(KLASS_DATABASE + name + QStringLiteral("/ChangeLog.txt"));
        if (!local.open(QIODevice::ReadOnly | QIODevice::Text))
            return true;

        const QString localTimestamp = extractTimestamp(QString::fromUtf8(local.read(1024)));
        local.close();

        if (localTimestamp.isEmpty() || remoteTimestamp != localTimestamp) {
            Debug::msg("Update Available in " + name, "RepoUtils", {remoteTimestamp, DColor::Yellow});
            return true;
        }
        Debug::msg("Repository up to date: " + name, "RepoUtils", {localTimestamp, DColor::Green});
    }
    return false;
}

/**
 * @brief Loads the active state for official sub-branches (Pasture, Testing, Extra) from klass.conf.
 * @return An OfficialBranches struct representing the enabled state of each sub-branch.
 */
Mirrors::OfficialBranches Mirrors::loadOfficialBranches() {
    OfficialBranches branches;
    branches.pasture = RepoUtils::parseBool(RepoUtils::readConfigValue(QStringLiteral("OFFICIAL_PASTURE=")));
    branches.testing = RepoUtils::parseBool(RepoUtils::readConfigValue(QStringLiteral("OFFICIAL_TESTING=")));
    branches.extra = RepoUtils::parseBool(RepoUtils::readConfigValue(QStringLiteral("OFFICIAL_EXTRA=")));
    return branches;
}

/**
 * @brief Writes official branches state under '# Official Repo' in klass.conf and cleans disabled repo directories.
 * @param branches An OfficialBranches struct containing the updated state.
 * @return True on success.
 */
bool Mirrors::setOfficialBranches(const OfficialBranches &branches) {
    auto saveBranch = [](const QString &key, const bool enabled, const QString &dirName) {
        const QString targetLine = key + u'=' + (enabled ? QStringLiteral("true") : QStringLiteral("false"));
        RepoUtils::setConfigValue(OFFICIAL_HEADER, key + u'=', targetLine);

        if (!enabled) {
            if (QDir dir(KLASS_DATABASE + dirName); dir.exists())
                dir.removeRecursively();
        }
    };

    saveBranch(QStringLiteral("OFFICIAL_PASTURE"), branches.pasture, SLACK_PASTURE);
    saveBranch(QStringLiteral("OFFICIAL_TESTING"), branches.testing, SLACK_TESTING);
    saveBranch(QStringLiteral("OFFICIAL_EXTRA"), branches.extra, SLACK_EXTRA);

    return true;
}

/**
 * @brief Extracts the first non-decorative timestamp line from a ChangeLog buffer.
 * @param text Raw ChangeLog content.
 * @return The trimmed timestamp line, or an empty QString if none found.
 */
QString Mirrors::extractTimestamp(const QString &text) {
    for (const auto lines = qTokenize(text, u'\n'); QStringView line: lines) {
        if (const QStringView l = line.trimmed(); !l.isEmpty() && !l.startsWith(u'+') && !l.startsWith(u'='))
            return l.toString();
    }
    return {};
}
