/**
 * @file Packages.hpp
 * @brief Header for config packages.
 */

#ifndef PACKAGES_HPP
#define PACKAGES_HPP

#include <QMap>
#include <QList>

#include <Debug.hpp>

/**
 * @struct PkgInfo
 * @brief Holds metadata for a package available in remote repositories.
 */
struct PkgInfo {
    bool isInstalled = false;
    QString name{};
    QString version{};
    QString description{};
    QString detailedDesc{};
    QString category{};
    QString compressedSize{};
    QString uncompressedSize{};
    QString required{};
    QString conflicts{};
    QString suggests{};
    QString repoName{};
};

/**
 * @struct RepoData
 * @brief Stores configuration data and package information for a repository.
 */
struct RepoData {
    bool hasCategories = false;
    QString mirrorUrl{};
    QList<PkgInfo> packages{};
};

/**
 * @struct ChecksumEntry
 * @brief Holds the resolved MD5 checksum and the original relative path parsed from CHECKSUMS.md5.
 */
struct ChecksumEntry {
    QString md5{};
    QString relativePath{};
};

/**
 * @enum FileFormat
 * @brief Identifies the file extension category we want to search for in the repository.
 */
enum class FileFormat {
    Package,
    Asc
};

/**
 * @class Packages
 * @brief Utility manager for scanning, parsing, and listing software packages in the Klass system.
 */
class Packages {
public:
    explicit Packages();

    void updateChecksums();

    void reload();

    [[nodiscard]] ChecksumEntry getChecksumEntry(const QString &repo, const QString &package,
                                                 const QString &version, FileFormat format = FileFormat::Package) const;

    [[nodiscard]] QMap<QString, RepoData> getAvailablePackages() const { return availableCache; }

    [[nodiscard]] QList<PkgInfo> getInstalledPackages() const { return installedCache; }

    [[nodiscard]] PkgInfo getPackageInfo(const QString &repo, const QString &package, const QString &version) const;

    static QStringList listPackageFiles(const QString &name, const QString &version);

private:
    void loadAvailablePackages();

    void loadInstalledPackages();

    static void parsePackagesContent(const QString &content, QList<PkgInfo> &packageList);

    Debug::Debug *debug{};
    QMap<QString, QMap<QString, ChecksumEntry> > checksumCache{};
    QList<PkgInfo> installedCache{};
    QMap<QString, RepoData> availableCache{};
};

#endif
