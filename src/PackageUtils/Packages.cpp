/**
 * @file Packages.cpp
 * @brief Implementation of the Packages class for managing remote and local Slackware packages.
 */

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStringView>

#include <Debug.hpp>
#include <Mirrors.hpp>
#include <Packages.hpp>
#include <RepoUtils.hpp>
#include <SlackwareDefines.hpp>
#include <Utils.hpp>

/**
 * @brief Constructs the Packages manager and triggers initial catalog load.
 * @param object Parent QObject.
 */
Packages::Packages(QObject *object) : QObject(object) {
    loadPackages();
}

/**
 * @brief Fast O(1) package name and version splitter from a canonical Slackware package string.
 * @details Format expected: name-version-arch-build (e.g. "aaa_base-15.0-x86_64-1").
 * @param rawName The raw package filename without archive extensions.
 * @param outName Output reference receiving the extracted base package name.
 * @param outVersion Output reference receiving the version string (version-arch-build).
 * @return True if the string matches Slackware delimiter formatting.
 */
bool Packages::splitPackageParts(const QStringView rawName, QString &outName, QString &outVersion) {
    const int lastDash = static_cast<int>(rawName.lastIndexOf(u'-'));
    if (lastDash <= 0) return false;

    const int archDash = static_cast<int>(rawName.lastIndexOf(u'-', lastDash - 1));
    if (archDash <= 0) return false;

    const int verDash = static_cast<int>(rawName.lastIndexOf(u'-', archDash - 1));
    if (verDash <= 0) return false;

    outName = rawName.left(verDash).toString();
    outVersion = QStringLiteral("%1-%2-%3")
            .arg(rawName.mid(verDash + 1, archDash - verDash - 1).toString(),
                 rawName.mid(archDash + 1, lastDash - archDash - 1).toString(),
                 rawName.mid(lastDash + 1).toString());
    return true;
}

/**
 * @brief Parses compressed or plain CHECKSUMS.md5 files.
 * @param repoPath Absolute filesystem path of the repository containing checksum files.
 * @return A QHash mapping relative package filenames to their ChecksumEntry records.
 */
QHash<QString, ChecksumEntry> Packages::parseRepoChecksums(const QString &repoPath) {
    const QString gzPath = repoPath + QStringLiteral("CHECKSUMS.md5.gz");
    const QString txtPath = repoPath + QStringLiteral("CHECKSUMS.md5");

    QString content;
    if (QFile::exists(gzPath)) {
        content = Utils::readGzipFile(gzPath);
    } else if (QFile::exists(txtPath)) {
        if (QFile file(txtPath); file.open(QIODevice::ReadOnly))
            content = QString::fromUtf8(file.readAll());
    }

    if (content.isEmpty())
        return {};

    QHash<QString, ChecksumEntry> result;
    result.reserve(4000);

    for (const auto lines = qTokenize(content, u'\n'); QStringView line: lines) {
        const QStringView trimmed = line.trimmed();
        if (trimmed.size() < 35 || trimmed.at(32) != u' ')
            continue;

        qsizetype pathStart = 33;
        while (pathStart < trimmed.size() && trimmed.at(pathStart) == u' ')
            ++pathStart;

        if (pathStart >= trimmed.size())
            continue;

        QStringView pathView = trimmed.mid(pathStart);
        if (pathView.startsWith(QLatin1String("./")))
            pathView = pathView.mid(2);

        const bool isPkg = RepoUtils::isPackageArchive(pathView);
        const bool isAsc = pathView.endsWith(u".asc");

        if (!isPkg && !isAsc)
            continue;

        const int lastSlash = static_cast<int>(pathView.lastIndexOf(u'/'));
        const QStringView keyView = (lastSlash != -1) ? pathView.mid(lastSlash + 1) : pathView;

        result.insert(keyView.toString(), {trimmed.left(32).toString(), pathView.toString()});
    }

    result.squeeze();
    return result;
}

/**
 * @brief Instantly retrieves the MD5 and relative path of a package from RAM.
 * @param repo Name of the repository directory.
 * @param package Name of the package.
 * @param version Complete version string.
 * @param format Requested file format extension to find.
 * @return Structure containing the MD5 and path.
 */
ChecksumEntry Packages::getChecksumEntry(const QString &repo, const QString &package,
                                         const QString &version, const FileFormat format) const {
    const auto repoIt = checksumCache.find(repo);
    if (repoIt == checksumCache.end())
        return ChecksumEntry{};

    const auto &repoCache = repoIt.value();
    const QString baseKey = package + u'-' + version;

    auto findChecksum = [&](const std::initializer_list<const char *> &extensions) -> ChecksumEntry {
        for (const char *ext: extensions) {
            if (const auto it = repoCache.find(baseKey + QString::fromLatin1(ext)); it != repoCache.end())
                return it.value();
        }
        return ChecksumEntry{};
    };

    if (format == FileFormat::Package)
        return findChecksum({".txz", ".tgz", ".tlz", ".tbz"});

    if (format == FileFormat::Asc)
        return findChecksum({".txz.asc", ".tgz.asc", ".tlz.asc", ".tbz.asc"});

    return ChecksumEntry{};
}

/**
 * @brief Instantly accesses metadata for a specific package from RAM.
 * @param repo Name of the repository directory.
 * @param package Name of the package.
 * @param version Complete version string.
 * @return Structure containing metadata or empty fields if not matched.
 */
PkgInfo Packages::getPackageInfo(const QString &repo, const QString &package, const QString &version) const {
    if (!repo.isEmpty() && repo != SLACK_OTHERS) {
        if (availableCache.contains(repo)) {
            for (const auto &pkg: availableCache.value(repo).packages) {
                if (pkg.name == package && pkg.version == version)
                    return pkg;
            }
        }
    }

    for (const auto &pkg: orphanCache) {
        if (pkg.name == package && pkg.version == version)
            return pkg;
    }

    if (repo.isEmpty()) {
        for (const auto &repoData: availableCache) {
            for (const auto &pkg: repoData.packages) {
                if (pkg.name == package && pkg.version == version)
                    return pkg;
            }
        }
    }
    return PkgInfo{};
}

/**
 * @brief Scans and retrieves available packages from local system and active repositories.
 */
void Packages::loadPackages() {
    availableCache.clear();
    checksumCache.clear();
    orphanCache.clear();
    installedNamesCache.clear();

    QHash<QString, QString> localPkgVersions, localPkgPaths;
    if (QDir(SLACK_PACKAGES).exists()) {
        QDirIterator itLocal(SLACK_PACKAGES, QDir::Files, QDirIterator::NoIteratorFlags);
        while (itLocal.hasNext()) {
            itLocal.next();
            QString pkgName, pkgVer;
            if (splitPackageParts(itLocal.fileName(), pkgName, pkgVer)) {
                localPkgVersions.insert(pkgName, pkgVer);
                localPkgPaths.insert(pkgName, itLocal.filePath());
                installedNamesCache.insert(pkgName);
            }
        }
    } else {
        Debug::msg("PkgTools Directory not Found", "Packages", {SLACK_PACKAGES, DColor::LightRed});
    }

    QSet<QString> packagesFoundInRepos;

    if (QDir(KLASS_DATABASE).exists()) {
        const Mirrors::MirrorEntry official = Mirrors::activeOne();
        const QList<Mirrors::MirrorPlusEntry> thirdPartyList = Mirrors::listActive();
        const auto branches = Mirrors::loadOfficialBranches();

        QMap<QString, QString> thirdPartyMap;
        for (const auto &entry: thirdPartyList)
            thirdPartyMap.insert(entry.name, entry.url);

        QSet<QString> activeRepos;
        if (!official.url.isEmpty()) {
            activeRepos.insert(SLACK_OFICIAL);
            activeRepos.insert(SLACK_PATCHES);
            if (branches.extra)
                activeRepos.insert(SLACK_EXTRA);
            if (branches.testing)
                activeRepos.insert(SLACK_TESTING);
            if (branches.pasture)
                activeRepos.insert(SLACK_PASTURE);
        }
        for (const auto &entry: thirdPartyList)
            activeRepos.insert(entry.name);

        QDirIterator itRepo(KLASS_DATABASE, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);

        while (itRepo.hasNext()) {
            itRepo.next();
            const QString repoName = itRepo.fileName();
            if (!activeRepos.contains(repoName))
                continue;

            const QString repoPath = itRepo.filePath() + u'/';
            if (auto checksums = parseRepoChecksums(repoPath); !checksums.isEmpty())
                checksumCache.insert(repoName, checksums);

            const QString txtPath = repoPath + QStringLiteral("PACKAGES.TXT");
            const QString gzPath = repoPath + QStringLiteral("PACKAGES.TXT.gz");

            QString fileContent;
            if (QFile::exists(gzPath))
                fileContent = Utils::readGzipFile(gzPath);
            else if (QFile::exists(txtPath)) {
                if (QFile file(txtPath); file.open(QIODevice::ReadOnly))
                    fileContent = QString::fromUtf8(file.readAll());
            }

            if (fileContent.isEmpty())
                continue;

            QList<PkgInfo> packageList;
            parsePackagesContent(fileContent, packageList);

            bool hasCats = false;
            for (PkgInfo &pkg: packageList) {
                pkg.repoName = repoName;
                if (!pkg.category.isEmpty())
                    hasCats = true;

                if (localPkgVersions.value(pkg.name) == pkg.version) {
                    pkg.isInstalled = true;
                    packagesFoundInRepos.insert(pkg.name);
                }
            }

            QString resolvedUrl;
            if (Utils::isOfficialRepo(repoName))
                resolvedUrl = Mirrors::branchUrl(official.url, repoName);
            else
                resolvedUrl = thirdPartyMap.value(repoName, QString{});

            availableCache.insert(repoName, {hasCats, resolvedUrl, std::move(packageList)});
        }
    } else {
        Debug::msg("Base Directory not Found", "Packages", {KLASS_DATABASE, DColor::LightRed});
    }

    orphanCache.reserve(localPkgPaths.size() - packagesFoundInRepos.size());
    for (auto it = localPkgPaths.constBegin(); it != localPkgPaths.constEnd(); ++it) {
        const QString &pkgName = it.key();

        if (packagesFoundInRepos.contains(pkgName))
            continue;

        PkgInfo pkg;
        pkg.isInstalled = true;
        pkg.name = pkgName;
        pkg.version = localPkgVersions.value(pkgName);
        pkg.repoName = SLACK_OTHERS;

        if (QFile file(it.value()); file.open(QIODevice::ReadOnly)) {
            const QString fileContent = QString::fromUtf8(file.readAll());
            file.close();

            const QString descPrefix = pkg.name + u':';
            bool capturandoDesc = false;

            for (auto lines = qTokenize(fileContent, u'\n'); QStringView lineToken: lines) {
                const QStringView line = lineToken.trimmed();
                if (line.startsWith(u"PACKAGE LOCATION:")) {
                    QStringView loc = RepoUtils::stripPackageExtension(line.mid(17).trimmed());
                    while (loc.endsWith(u'/'))
                        loc = loc.chopped(1);

                    const int lastSlash = static_cast<int>(loc.lastIndexOf(u'/'));
                    const QStringView folder = (lastSlash != -1) ? loc.mid(lastSlash + 1) : loc;
                    if (!folder.isEmpty() && folder != u"." && folder != u".." && !folder.contains(pkg.name))
                        pkg.category = folder.toString();
                } else if (line.startsWith(u"COMPRESSED PACKAGE SIZE:")) {
                    pkg.compressedSize = line.mid(24).trimmed().toString();
                } else if (line.startsWith(u"UNCOMPRESSED PACKAGE SIZE:")) {
                    pkg.uncompressedSize = line.mid(26).trimmed().toString();
                } else if (line.startsWith(u"PACKAGE DESCRIPTION:")) {
                    capturandoDesc = true;
                } else if (capturandoDesc) {
                    if (lineToken.startsWith(descPrefix, Qt::CaseInsensitive)) {
                        RepoUtils::appendPackageDescription(lineToken, descPrefix, pkg.description, pkg.detailedDesc);
                    } else if (line.isEmpty() && !pkg.detailedDesc.isEmpty()) {
                        capturandoDesc = false;
                    }
                }
            }
            if (!pkg.detailedDesc.isEmpty())
                pkg.detailedDesc = pkg.detailedDesc.trimmed();
        }
        orphanCache.append(std::move(pkg));
    }
}

/**
 * @brief Parses the installation log of a package and returns its installed files.
 * @param name The name of the package.
 * @param version The complete version string.
 * @return QStringList containing paths of installed files.
 */
QStringList Packages::listPackageFiles(const QString &name, const QString &version) {
    QStringList files;
    const QString logPath = SLACK_PACKAGES + name + u'-' + version;
    QFile logFile(logPath);

    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return files;

    QTextStream stream(&logFile);
    const QString prefixoDesc = name + u':';

    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        const QString trimmed = line.trimmed();

        if (trimmed.isEmpty() || trimmed.startsWith(prefixoDesc) || trimmed.contains(u':'))
            continue;

        if (!trimmed.endsWith(u'/'))
            files.append(line);
    }

    logFile.close();
    return files;
}

/**
 * @brief Parses raw PACKAGES.TXT content, extracts package metadata, and builds package lists.
 * @param content The plain text document containing package descriptors.
 * @param packageList Target container receiving parsed package structures.
 */
void Packages::parsePackagesContent(const QString &content, QList<PkgInfo> &packageList) {
    if (content.isEmpty())
        return;

    packageList.reserve(1500);
    PkgInfo currentPkg;
    QString currentDescPrefix;
    bool processandoPacote = false, capturandoDesc = false;

    for (auto lines = qTokenize(content, u'\n'); QStringView line: lines) {
        const QStringView trimmedLine = line.trimmed();

        if (trimmedLine.startsWith(u"PACKAGE NAME:")) {
            if (processandoPacote && !currentPkg.name.isEmpty()) {
                if (!currentPkg.detailedDesc.isEmpty())
                    currentPkg.detailedDesc = currentPkg.detailedDesc.trimmed();
                packageList.append(std::move(currentPkg));
            }
            currentPkg = PkgInfo();
            processandoPacote = true;
            capturandoDesc = false;

            const QStringView rawName = RepoUtils::stripPackageExtension(trimmedLine.mid(13).trimmed());
            if (splitPackageParts(rawName, currentPkg.name, currentPkg.version))
                currentDescPrefix = currentPkg.name + u':';
            continue;
        }

        if (!processandoPacote)
            continue;

        if (trimmedLine.startsWith(u"PACKAGE LOCATION:")) {
            const QStringView loc = trimmedLine.mid(17).trimmed();
            const int lastSlash = static_cast<int>(loc.lastIndexOf(u'/'));
            currentPkg.category = (lastSlash != -1 ? loc.mid(lastSlash + 1) : loc).toString();
        } else if (trimmedLine.startsWith(u"PACKAGE SIZE (compressed):")) {
            currentPkg.compressedSize = trimmedLine.mid(26).trimmed().toString();
        } else if (trimmedLine.startsWith(u"PACKAGE SIZE (uncompressed):")) {
            currentPkg.uncompressedSize = trimmedLine.mid(28).trimmed().toString();
        } else if (trimmedLine.startsWith(u"PACKAGE REQUIRED:")) {
            currentPkg.required = trimmedLine.mid(17).trimmed().toString();
        } else if (trimmedLine.startsWith(u"PACKAGE CONFLICTS:")) {
            currentPkg.conflicts = trimmedLine.mid(18).trimmed().toString();
        } else if (trimmedLine.startsWith(u"PACKAGE SUGGESTS:")) {
            currentPkg.suggests = trimmedLine.mid(17).trimmed().toString();
        } else if (!currentDescPrefix.isEmpty() && line.startsWith(currentDescPrefix, Qt::CaseInsensitive)) {
            RepoUtils::appendPackageDescription(line, currentDescPrefix, currentPkg.description,
                                                currentPkg.detailedDesc);
            capturandoDesc = true;
        } else if (capturandoDesc && trimmedLine.isEmpty()) {
            capturandoDesc = false;
        }
    }

    if (processandoPacote && !currentPkg.name.isEmpty()) {
        if (!currentPkg.detailedDesc.isEmpty())
            currentPkg.detailedDesc = currentPkg.detailedDesc.trimmed();
        packageList.append(std::move(currentPkg));
    }
}
