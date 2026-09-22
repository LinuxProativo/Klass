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
 * @brief Dynamic Structured Constructor. Isolates memory profiles according to selected InitMode strategy.
 */
Packages::Packages() {
    debug = new Debug::Debug();
    updateChecksums();
    loadInstalledPackages();
    loadAvailablePackages();
}

/**
 * @brief Reloads all package databases, checksums, and local installation states into RAM.
 */
void Packages::reload() {
    updateChecksums();
    loadInstalledPackages();
    loadAvailablePackages();
}

/**
 * @brief Reloads and updates all repository checksum files (CHECKSUMS.{md5/md5.gz}) into RAM.
 */
void Packages::updateChecksums() {
    checksumCache.clear();
    if (!QDir(KLASS_DATABASE).exists()) {
        debug->msg("Base Directory not Found for Checksums", "Packages", {KLASS_DATABASE, Debug::LightRed});
        return;
    }

    QDirIterator it(KLASS_DATABASE, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        it.next();
        QString repoName = it.fileName();
        QString repoPath = it.filePath() + u'/';

        QString txtPath = repoPath + QStringLiteral("CHECKSUMS.md5");
        QString gzPath = repoPath + QStringLiteral("CHECKSUMS.md5.gz");

        QString fileContent;
        if (QFile::exists(gzPath)) {
            fileContent = Utils::readGzipFile(gzPath);
        } else if (QFile::exists(txtPath)) {
            if (QFile file(txtPath); file.open(QIODevice::ReadOnly)) {
                fileContent = QString::fromUtf8(file.readAll());
                file.close();
            }
        }

        if (fileContent.isEmpty())
            continue;

        QMap<QString, ChecksumEntry> repoChecksums;

        for (auto lines = qTokenize(fileContent, u'\n'); QStringView line: lines) {
            QStringView trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;

            auto tokens = qTokenize(trimmed, u' ');
            QList<QStringView> tokenList;
            for (QStringView t: tokens) {
                if (!t.isEmpty())
                    tokenList.append(t);
            }

            if (tokenList.size() < 2) continue;

            QString md5 = tokenList.first().toString();
            if (md5.length() != 32) continue;

            QString rawPath = tokenList.last().toString();
            QString cleanRelativePath = rawPath;
            if (cleanRelativePath.startsWith(QLatin1String("./")))
                cleanRelativePath = cleanRelativePath.mid(2);

            int lastSlash = static_cast<int>(cleanRelativePath.lastIndexOf(u'/'));
            QString key = (lastSlash != -1) ? cleanRelativePath.mid(lastSlash + 1) : cleanRelativePath;

            ChecksumEntry entry;
            entry.md5 = md5;
            entry.relativePath = cleanRelativePath;
            repoChecksums.insert(key, entry);
        }

        checksumCache.insert(repoName, repoChecksums);
        debug->msg("Checksums Loaded for Repo: " + repoName, "Packages", {QString::number(repoChecksums.size())});
    }
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
    if (!checksumCache.contains(repo))
        return ChecksumEntry{};

    const auto &repoCache = checksumCache.value(repo);
    const QString baseKey = package + u'-' + version;

    if (format == FileFormat::Package) {
        static const std::array packageExtensions = {
            QStringLiteral(".txz"), QStringLiteral(".tgz"), QStringLiteral(".tlz"), QStringLiteral(".tbz")
        };

        for (const QString &ext: packageExtensions) {
            if (QString queryKey = baseKey + ext; repoCache.contains(queryKey))
                return repoCache.value(queryKey);
        }
    } else if (format == FileFormat::Asc) {
        QString queryKey = baseKey + QStringLiteral(".txz.asc");
        if (repoCache.contains(queryKey))
            return repoCache.value(queryKey);

        static const std::array otherAscExtensions = {
            QStringLiteral(".tgz.asc"), QStringLiteral(".tlz.asc"), QStringLiteral(".tbz.asc")
        };
        for (const QString &ext: otherAscExtensions) {
            queryKey = baseKey + ext;
            if (repoCache.contains(queryKey))
                return repoCache.value(queryKey);
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

    if (!QDir(KLASS_DATABASE).exists()) {
        debug->msg("Base Directory not Found", "Packages", {KLASS_DATABASE, Debug::LightRed});
        return;
    }

    const Mirrors::MirrorEntry official = Mirrors::activeOne();
    const QList<Mirrors::MirrorPlusEntry> thirdPartyList = Mirrors::listActive();

    QMap<QString, QString> thirdPartyMap;
    for (const auto &entry: thirdPartyList)
        thirdPartyMap.insert(entry.name, entry.url);

    QSet<std::pair<QString, QString> > installedSet;
    installedSet.reserve(installedCache.size());

    for (const auto &pkg: installedCache)
        installedSet.insert({pkg.name, pkg.version});

    QDirIterator it(KLASS_DATABASE, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        it.next();
        QString repoName = it.fileName();
        QString repoPath = it.filePath() + u'/';

        QString txtPath = repoPath + "PACKAGES.TXT";
        QString gzPath = repoPath + "PACKAGES.TXT.gz";

        QString fileContent;
        if (QFile::exists(gzPath)) {
            fileContent = Utils::readGzipFile(gzPath);
        } else if (QFile::exists(txtPath)) {
            if (QFile file(txtPath); file.open(QIODevice::ReadOnly)) {
                fileContent = QString::fromUtf8(file.readAll());
                file.close();
            }
        }

        if (!fileContent.isEmpty()) {
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

            QString resolvedUrl{};
            if (repoName == SLACK_OFICIAL) {
                resolvedUrl = official.url;
            } else if (repoName == SLACK_PATCHES) {
                resolvedUrl = official.url + QStringLiteral("patches/");
            } else if (repoName == SLACK_EXTRA) {
                resolvedUrl = official.url + QStringLiteral("extra/");
            } else if (repoName == SLACK_TESTING) {
                resolvedUrl = official.url + QStringLiteral("testing/");
            } else {
                resolvedUrl = thirdPartyMap.value(repoName, QString{});
            }

            debug->msg("Category Found in " + repoName, "Packages", {hasCats ? "true" : "false"});
            availableCache.insert(repoName, {hasCats, resolvedUrl, std::move(packageList)});
        }
    }
}

/**
 * @brief Lists all packages currently installed on the local system, including their short descriptions.
 */
void Packages::loadInstalledPackages() {
    installedCache.clear();

    if (const QDir dir(SLACK_PACKAGES); !dir.exists()) {
        debug->msg("PkgTools Directory not Found", "Packages", {SLACK_PACKAGES, Debug::LightRed});
        return;
    }

    QDirIterator it(SLACK_PACKAGES, QDir::Files, QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        it.next();
        QString pkgFileName = it.fileName();

        auto tokens = qTokenize(pkgFileName, u'-');
        QList<QStringView> tokenList;
        for (QStringView token: tokens)
            tokenList.append(token);

        if (tokenList.size() < 4)
            continue;

        QStringView buildRev = tokenList.takeLast();
        QStringView arch = tokenList.takeLast();
        QStringView version = tokenList.takeLast();

        PkgInfo pkg;
        pkg.isInstalled = true;
        QStringList nameParts;
        for (QStringView t: tokenList)
            nameParts.append(t.toString());

        pkg.name = nameParts.join(u'-');
        pkg.version = QStringLiteral("%1-%2-%3").arg(version.toString(), arch.toString(), buildRev.toString());

        if (QFile file(it.filePath()); file.open(QIODevice::ReadOnly)) {
            QString fileContent = QString::fromUtf8(file.readAll());
            file.close();

            const QString prefixoDescricao = pkg.name + ":";
            bool capturandoDesc = false;

            for (auto lines = qTokenize(fileContent, u'\n'); QStringView lineToken: lines) {
                if (QStringView line = lineToken.trimmed(); line.startsWith(u"PACKAGE LOCATION:")) {
                    QString loc = line.mid(17).trimmed().toString();

                    if (loc.endsWith(u".txz") || loc.endsWith(u".tgz") ||
                        loc.endsWith(u".tbz") || loc.endsWith(u".tlz")) {
                        if (int lastSlash = static_cast<int>(loc.lastIndexOf(u'/')); lastSlash != -1) {
                            loc = loc.left(lastSlash);
                        } else {
                            loc.clear();
                        }
                    }

                    while (loc.endsWith(u'/'))
                        loc.chop(1);

                    int lastSlash = static_cast<int>(loc.lastIndexOf(u'/'));
                    QString folder = (lastSlash != -1) ? loc.mid(lastSlash + 1) : loc;
                    if (!folder.isEmpty() && folder != u"." && folder != u".." && !folder.contains(pkg.name))
                        pkg.category = folder;
                } else if (line.startsWith(u"COMPRESSED PACKAGE SIZE:")) {
                    pkg.compressedSize = line.mid(24).trimmed().toString();
                } else if (line.startsWith(u"UNCOMPRESSED PACKAGE SIZE:")) {
                    pkg.uncompressedSize = line.mid(26).trimmed().toString();
                } else if (line.startsWith(u"PACKAGE DESCRIPTION:")) {
                    capturandoDesc = true;
                } else if (capturandoDesc) {
                    if (lineToken.startsWith(prefixoDescricao, Qt::CaseInsensitive)) {
                        QString contentStr = Utils::normalizeSpaces(lineToken.mid(prefixoDescricao.length()));

                        if (pkg.description.isEmpty() && !contentStr.trimmed().isEmpty()) {
                            QString trimmedContent = contentStr.trimmed();
                            int openParen = static_cast<int>(trimmedContent.indexOf(u'('));
                            if (int closeParen = static_cast<int>(trimmedContent.indexOf(u')'));
                                openParen != -1 && closeParen > openParen) {
                                pkg.description = trimmedContent.mid(openParen + 1, closeParen - openParen - 1);
                            } else {
                                pkg.description = trimmedContent;
                            }
                        }

                        if (!contentStr.isEmpty()) {
                            pkg.detailedDesc += contentStr + u'\n';
                        } else if (!pkg.detailedDesc.isEmpty() && !pkg.detailedDesc.endsWith(u"\n\n")) {
                            pkg.detailedDesc += u"\n";
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
        QString line = stream.readLine();
        QString trimmed = line.trimmed();

        if (trimmed.isEmpty() || trimmed.startsWith(prefixoDesc) || trimmed.contains(u":"))
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

    PkgInfo currentPkg;
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

            auto tokens = qTokenize(rawName, u'-');
            QList<QStringView> tokenList;
            for (QStringView token: tokens)
                tokenList.append(token);

            if (tokenList.size() >= 4) {
                QStringView buildRev = tokenList.takeLast();
                QStringView arch = tokenList.takeLast();
                QStringView version = tokenList.takeLast();
                QStringList nameParts;
                for (QStringView t: tokenList)
                    nameParts.append(t.toString());

                currentPkg.name = nameParts.join(u'-');
                currentPkg.version = QStringLiteral("%1-%2-%3").arg(version, arch, buildRev);
            }
            continue;
        }

        if (!processandoPacote)
            continue;

        if (trimmedLine.startsWith(u"PACKAGE LOCATION:")) {
            QStringView loc = trimmedLine.mid(17).trimmed();
            int lastSlash = static_cast<int>(loc.lastIndexOf(u'/'));
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
        } else if (!capturandoDesc && line.startsWith(currentPkg.name + u':', Qt::CaseInsensitive)) {
            QString contentStr = Utils::normalizeSpaces(line.mid(currentPkg.name.length() + 1));
            QString trimmedContent = contentStr.trimmed();
            int openParen = static_cast<int>(trimmedContent.indexOf(u'('));
            if (int closeParen = static_cast<int>(trimmedContent.lastIndexOf(u')'));
                openParen != -1 && closeParen > openParen) {
                currentPkg.description = trimmedContent.mid(openParen + 1, closeParen - openParen - 1);
            } else {
                currentPkg.description = trimmedContent;
            }

            currentPkg.detailedDesc = contentStr + u'\n';
            capturandoDesc = true;
        } else if (capturandoDesc && line.startsWith(currentPkg.name + u':', Qt::CaseInsensitive)) {
            if (QString contentStr = Utils::normalizeSpaces(line.mid(currentPkg.name.length() + 1)); !contentStr.
                isEmpty()) {
                currentPkg.detailedDesc += contentStr + u'\n';
            } else if (!currentPkg.detailedDesc.isEmpty() && !currentPkg.detailedDesc.endsWith(u"\n\n")) {
                currentPkg.detailedDesc += u"\n";
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
