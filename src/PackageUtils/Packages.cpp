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
#include <SlackwareDefines.hpp>
#include <Utils.hpp>

/**
 * @brief Dynamic Structured Constructor.
 */
Packages::Packages(QObject *object) : QObject(object) {
    reload();
}

/**
 * @brief Reloads all package databases, checksums, and local installation states into RAM.
 */
void Packages::reload() {
    loadInstalledPackages();
    loadAvailablePackages();
}

/**
 * @brief Fast O(1) package name and version splitter from a canonical Slackware package string.
 * @details Format expected: name-version-arch-build (e.g., "aaa_base-15.0-x86_64-1").
 * @param rawName The raw package filename or identifier string view without archive extensions.
 * @param outName Output reference that receives the extracted package base name.
 * @param outVersion Output reference that receives the reconstructed version string (version-arch-build).
 * @return True if the string contains the expected delimiters and was successfully split, false otherwise.
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
 * @brief Parses compressed or plain CHECKSUMS.md5 files without temporary allocations.
 * @param repoPath Absolute filesystem path of the repository containing checksum files.
 * @return A QHash mapping relative package file keys to their ChecksumEntry records.
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
    result.reserve(15000);

    for (const auto lines = qTokenize(content, u'\n'); QStringView line: lines) {
        QStringView trimmed = line.trimmed();
        if (trimmed.size() < 35)
            continue;

        const QStringView md5View = trimmed.left(32);
        if (trimmed.at(32) != u' ')
            continue;

        qsizetype pathStart = 33;
        while (pathStart < trimmed.size() && trimmed.at(pathStart) == u' ')
            ++pathStart;

        if (pathStart >= trimmed.size())
            continue;

        QStringView pathView = trimmed.mid(pathStart);
        if (pathView.startsWith(QLatin1String("./")))
            pathView = pathView.mid(2);

        const int lastSlash = static_cast<int>(pathView.lastIndexOf(u'/'));
        const QStringView keyView = lastSlash != -1 ? pathView.mid(lastSlash + 1) : pathView;

        result.insert(keyView.toString(), {md5View.toString(), pathView.toString()});
    }

    return result;
}

/**
 * @brief Instantly retrieves both the MD5 and the original relative path of a package from RAM.
 * @param repo Name of the repository directory.
 * @param package Name of the package (e.g., "aspell-ar").
 * @param version Complete version string (e.g., "1.2_0-x86_64-1").
 * @param format The requested file format extension to search for.
 * @return Structure containing the MD5 and the path. Returns empty strings if not found.
 */
ChecksumEntry Packages::getChecksumEntry(const QString &repo, const QString &package,
                                         const QString &version, const FileFormat format) const {
    const auto repoIt = checksumCache.find(repo);
    if (repoIt == checksumCache.end())
        return ChecksumEntry{};

    const auto &repoCache = repoIt.value();
    const QString baseKey = package + u'-' + version;

    if (format == FileFormat::Package) {
        static const std::array packageExtensions = {
            QStringLiteral(".txz"), QStringLiteral(".tgz"), QStringLiteral(".tlz"), QStringLiteral(".tbz")
        };

        for (const QString &ext: packageExtensions) {
            if (const auto it = repoCache.find(baseKey + ext); it != repoCache.end())
                return it.value();
        }
    } else if (format == FileFormat::Asc) {
        if (const auto it = repoCache.find(baseKey + QStringLiteral(".txz.asc")); it != repoCache.end())
            return it.value();

        static const std::array otherAscExtensions = {
            QStringLiteral(".tgz.asc"), QStringLiteral(".tlz.asc"), QStringLiteral(".tbz.asc")
        };
        for (const QString &ext: otherAscExtensions) {
            if (const auto it2 = repoCache.find(baseKey + ext); it2 != repoCache.end())
                return it2.value();
        }
    }

    return ChecksumEntry{};
}

/**
 * @brief Instantly accesses metadata for a specific package from RAM, matching its repository, name, and version.
 * @param repo Name of the repository directory (use "Local" for local packages).
 * @param package Name of the package (e.g., "aspell-ar").
 * @param version Complete version string (e.g., "1.2_0-x86_64-1").
 * @return Structure containing metadata or empty fields if not matched.
 */
PkgInfo Packages::getPackageInfo(const QString &repo, const QString &package, const QString &version) const {
    if (repo.contains(SLACK_OTHERS) || repo.isEmpty()) {
        for (const auto &pkg: installedCache) {
            if (pkg.name == package && pkg.version == version)
                return pkg;
        }
    } else {
        if (availableCache.contains(repo)) {
            for (const auto &pkg: availableCache.value(repo).packages) {
                if (pkg.name == package && pkg.version == version)
                    return pkg;
            }
        }
    }
    return PkgInfo{};
}

/**
 * @brief Scans and retrieves available packages from all repositories found inside the database directory.
 */
void Packages::loadAvailablePackages() {
    availableCache.clear();
    checksumCache.clear();

    if (!QDir(KLASS_DATABASE).exists()) {
        Debug::msg("Base Directory not Found", "Packages", {KLASS_DATABASE, DColor::LightRed});
        return;
    }

    const Mirrors::MirrorEntry official = Mirrors::activeOne();
    const QList<Mirrors::MirrorPlusEntry> thirdPartyList = Mirrors::listActive();

    QMap<QString, QString> thirdPartyMap;
    for (const auto &entry: thirdPartyList)
        thirdPartyMap.insert(entry.name, entry.url);

    QSet<QString> activeRepos;
    if (!official.url.isEmpty()) {
        activeRepos.insert(SLACK_OFICIAL);
        activeRepos.insert(SLACK_PATCHES);
        activeRepos.insert(SLACK_EXTRA);
        activeRepos.insert(SLACK_TESTING);
    }
    for (const auto &entry: thirdPartyList)
        activeRepos.insert(entry.name);

    QSet<std::pair<QString, QString> > installedSet;
    installedSet.reserve(installedCache.size());

    for (const auto &pkg: installedCache)
        installedSet.insert({pkg.name, pkg.version});

    QDirIterator it(KLASS_DATABASE, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        it.next();
        const QString repoName = it.fileName();

        if (!activeRepos.contains(repoName))
            continue;

        const QString repoPath = it.filePath() + u'/';

        if (auto checksums = parseRepoChecksums(repoPath); !checksums.isEmpty())
            checksumCache.insert(repoName, checksums);

        const QString txtPath = repoPath + QStringLiteral("PACKAGES.TXT");
        const QString gzPath = repoPath + QStringLiteral("PACKAGES.TXT.gz");

        QString fileContent;
        if (QFile::exists(gzPath)) {
            fileContent = Utils::readGzipFile(gzPath);
        } else if (QFile::exists(txtPath)) {
            if (QFile file(txtPath); file.open(QIODevice::ReadOnly))
                fileContent = QString::fromUtf8(file.readAll());
        }

        if (fileContent.isEmpty())
            continue;

        QList<PkgInfo> packageList;
        parsePackagesContent(fileContent, packageList);

        for (PkgInfo &pkg: packageList) {
            pkg.repoName = repoName;
            if (installedSet.contains({pkg.name, pkg.version}))
                pkg.isInstalled = true;
        }

        bool hasCats = false;
        for (const auto &pkg: packageList) {
            if (!pkg.category.isEmpty()) {
                hasCats = true;
                break;
            }
        }

        QString resolvedUrl;
        if (repoName == SLACK_OFICIAL)
            resolvedUrl = official.url;
        else if (repoName == SLACK_PATCHES)
            resolvedUrl = official.url + QStringLiteral("patches/");
        else if (repoName == SLACK_EXTRA)
            resolvedUrl = official.url + QStringLiteral("extra/");
        else if (repoName == SLACK_TESTING)
            resolvedUrl = official.url + QStringLiteral("testing/");
        else
            resolvedUrl = thirdPartyMap.value(repoName, QString{});

        Debug::msg("Category Found in " + repoName, "Packages", {hasCats ? "true" : "false"});
        availableCache.insert(repoName, {hasCats, resolvedUrl, std::move(packageList)});
    }
}

/**
 * @brief Lists all packages currently installed on the local system, including their short descriptions.
 */
void Packages::loadInstalledPackages() {
    installedCache.clear();

    if (const QDir dir(SLACK_PACKAGES); !dir.exists()) {
        Debug::msg("PkgTools Directory not Found", "Packages", {SLACK_PACKAGES, DColor::LightRed});
        return;
    }

    QDirIterator it(SLACK_PACKAGES, QDir::Files, QDirIterator::NoIteratorFlags);
    installedCache.reserve(2000);

    while (it.hasNext()) {
        it.next();
        const QString pkgFileName = it.fileName();

        PkgInfo pkg;
        pkg.isInstalled = true;

        if (!splitPackageParts(pkgFileName, pkg.name, pkg.version))
            continue;

        if (QFile file(it.filePath()); file.open(QIODevice::ReadOnly)) {
            const QString fileContent = QString::fromUtf8(file.readAll());
            file.close();

            const QString descPrefix = pkg.name + u':';
            bool capturandoDesc = false;

            for (auto lines = qTokenize(fileContent, u'\n'); QStringView lineToken: lines) {
                if (QStringView line = lineToken.trimmed(); line.startsWith(u"PACKAGE LOCATION:")) {
                    QStringView loc = line.mid(17).trimmed();

                    if (loc.endsWith(u".txz") || loc.endsWith(u".tgz") ||
                        loc.endsWith(u".tbz") || loc.endsWith(u".tlz")) {
                        const int lastSlash = static_cast<int>(loc.lastIndexOf(u'/'));
                        loc = (lastSlash != -1) ? loc.left(lastSlash) : QStringView{};
                    }

                    while (loc.endsWith(u'/'))
                        loc = loc.chopped(1);

                    const int lastSlash = static_cast<int>(loc.lastIndexOf(u'/'));
                    const QStringView folder = lastSlash != -1 ? loc.mid(lastSlash + 1) : loc;
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
                        const QString contentStr = Utils::normalizeSpaces(lineToken.mid(descPrefix.length()));

                        if (pkg.description.isEmpty() && !contentStr.trimmed().isEmpty()) {
                            const QString trimmedContent = contentStr.trimmed();
                            const int openParen = static_cast<int>(trimmedContent.indexOf(u'('));
                            const int closeParen = static_cast<int>(trimmedContent.indexOf(u')'));
                            if (openParen != -1 && closeParen > openParen) {
                                pkg.description = trimmedContent.mid(openParen + 1, closeParen - openParen - 1);
                            } else {
                                pkg.description = trimmedContent;
                            }
                        }

                        if (!contentStr.isEmpty()) {
                            pkg.detailedDesc += contentStr + u'\n';
                        } else if (!pkg.detailedDesc.isEmpty() && !pkg.detailedDesc.endsWith(u"\n\n")) {
                            pkg.detailedDesc += u'\n';
                        }
                    } else if (line.isEmpty() && !pkg.detailedDesc.isEmpty()) {
                        capturandoDesc = false;
                    }
                }
            }
            if (!pkg.detailedDesc.isEmpty())
                pkg.detailedDesc = pkg.detailedDesc.trimmed();
        }
        installedCache.append(std::move(pkg));
    }
}

/**
 * @brief Parses the installation log of a package and returns its installed files.
 * @param name The name of the package.
 * @param version The complete version string (e.g., "1.0.0-x86_64-1").
 * @return QStringList containing the paths of the installed files.
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
 * @brief Parses raw text buffer contents, filters package identity tags, and extracts short descriptions.
 * @param content The raw, plain text string payload containing package descriptors.
 * @param packageList Target memory sequence pointer reference to hold appended discoveries.
 */
void Packages::parsePackagesContent(const QString &content, QList<PkgInfo> &packageList) {
    if (content.isEmpty())
        return;

    packageList.reserve(1500);
    PkgInfo currentPkg;
    QString currentDescPrefix;
    bool processandoPacote = false, capturandoDesc = false;

    for (auto lines = qTokenize(content, u'\n'); QStringView line: lines) {
        QStringView trimmedLine = line.trimmed();

        if (trimmedLine.startsWith(u"PACKAGE NAME:")) {
            if (processandoPacote && !currentPkg.name.isEmpty()) {
                if (!currentPkg.detailedDesc.isEmpty())
                    currentPkg.detailedDesc = currentPkg.detailedDesc.trimmed();
                packageList.append(std::move(currentPkg));
            }
            currentPkg = PkgInfo();
            processandoPacote = true;
            capturandoDesc = false;

            QStringView rawName = trimmedLine.mid(13).trimmed();
            if (rawName.endsWith(u".txz") || rawName.endsWith(u".tgz") ||
                rawName.endsWith(u".tbz") || rawName.endsWith(u".tlz"))
                rawName = rawName.chopped(4);

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
        } else if (!capturandoDesc && !currentDescPrefix.isEmpty() && line.startsWith(
                       currentDescPrefix, Qt::CaseInsensitive)) {
            const QString contentStr = Utils::normalizeSpaces(line.mid(currentDescPrefix.length()));
            const QString trimmedContent = contentStr.trimmed();
            const int openParen = static_cast<int>(trimmedContent.indexOf(u'('));
            const int closeParen = static_cast<int>(trimmedContent.lastIndexOf(u')'));
            if (openParen != -1 && closeParen > openParen) {
                currentPkg.description = trimmedContent.mid(openParen + 1, closeParen - openParen - 1);
            } else {
                currentPkg.description = trimmedContent;
            }

            currentPkg.detailedDesc = contentStr + u'\n';
            capturandoDesc = true;
        } else if (capturandoDesc && !currentDescPrefix.isEmpty() && line.startsWith(
                       currentDescPrefix, Qt::CaseInsensitive)) {
            if (const QString contentStr = Utils::normalizeSpaces(line.mid(currentDescPrefix.length())); !contentStr.
                isEmpty()) {
                currentPkg.detailedDesc += contentStr + u'\n';
            } else if (!currentPkg.detailedDesc.isEmpty() && !currentPkg.detailedDesc.endsWith(u"\n\n")) {
                currentPkg.detailedDesc += u'\n';
            }
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
