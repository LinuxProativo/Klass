/**
 * @file RepoManager.cpp
 * @brief Implementation of the RepoManager class.
 */

#include <QDir>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>

#include <Debug.hpp>
#include <DefaultPath.hpp>
#include <Mirrors.hpp>
#include <RepoManager.hpp>
#include <SlackwareDefines.hpp>

/**
 * @brief Atomically writes @p lines to @p path using QSaveFile.
 * @return True on success.
 */
bool RepoManager::writeLines(const QString &path, const QStringList &lines) {
    QSaveFile sf(path);
    if (!sf.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&sf);
    for (int i = 0; i < lines.size(); ++i) {
        out << lines[i];
        if (i < lines.size() - 1) {
            out << '\n';
        }
    }
    out << '\n';

    return sf.commit();
}

/**
 * @brief Searches for all ChangeLog.txt files within the new database directory structure.
 * @return A QStringList containing the absolute paths to all found ChangeLog files.
 */
QStringList RepoManager::fetchChangeLogs() {
    const QDir rootDir(KLASS_DATABASE);
    QStringList repos = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList foundFiles;

    for (const QString &repo: repos) {
        QDir repoDir(KLASS_DATABASE + repo);

        for (QStringList txtFiles = repoDir.entryList({"ChangeLog.txt"}, QDir::Files, QDir::Name);
             const QString &file: txtFiles) {
            foundFiles << repoDir.absoluteFilePath(file);
        }
    }

    return foundFiles;
}

/**
 * @brief Reads the configuration file. Creates it with standard headers if it does not exist.
 * @return QStringList containing the lines of the configuration file.
 */
QStringList RepoManager::readConf() {
    QFile f(CONFIG_PATH);

    if (!f.exists()) {
        QStringList defaultTemplate = {
            QStringLiteral("# Official Repo"),
            QStringLiteral(""),
            QStringLiteral("# Third Mirrors"),
            QStringLiteral(""),
            QStringLiteral("# Exceptions"),
            QStringLiteral(""),
            QStringLiteral("# Priorities")
        };
        if (writeConf(defaultTemplate))
            return defaultTemplate;
        return {};
    }

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QStringList lines;
    QTextStream in(&f);
    while (!in.atEnd())
        lines << in.readLine();
    return lines;
}

/**
 * @brief Writes the provided list of lines to the repository configuration file.
 * @param lines A QStringList containing the lines to be saved to the file.
 * @return True if the file was written successfully, false otherwise.
 */
bool RepoManager::writeConf(const QStringList &lines) {
    const QFileInfo fileInfo(CONFIG_PATH);
    if (const QDir dir = fileInfo.dir(); !dir.exists())
        (void) dir.mkpath(".");

    return writeLines(CONFIG_PATH, lines);
}
