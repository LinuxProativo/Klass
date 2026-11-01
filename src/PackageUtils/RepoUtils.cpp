/**
 * @file RepoUtils.cpp
 * @brief Implementation of static internal package and repository utility functions.
 */

#include <QDir>
#include <QSaveFile>

#include <RepoUtils.hpp>
#include <Utils.hpp>

namespace RepoUtils {
    /**
     * @brief Returns the machine architecture as used in mirror paths.
     * @return "x86_64", "x86", or an empty QString when undetermined.
     */
    QString detectArch() {
        QString arch = QSysInfo::buildCpuArchitecture();
        return arch == u"i686" || arch == u"i586" || arch == u"i486" || arch == u"i386" ? "x86" : arch;
    }

    /**
     * @brief Collapses runs of consecutive spaces into a single space and drops one leading space.
     * @param raw View over the raw text to normalize.
     * @return A newly built string with normalized internal whitespace.
     */
    QString normalizeSpaces(const QStringView raw) {
        if (raw.startsWith(u' '))
            return normalizeSpaces(raw.mid(1));

        QString normalized;
        normalized.reserve(raw.size());
        bool lastWasSpace = false;

        for (const QChar ch: raw) {
            if (ch == u' ') {
                if (!lastWasSpace) {
                    normalized.append(ch);
                    lastWasSpace = true;
                }
            } else {
                normalized.append(ch);
                lastWasSpace = false;
            }
        }

        normalized.squeeze();
        return normalized;
    }

    /**
     * @brief Atomically writes @p lines to @p path using QSaveFile.
     * @return True on success.
     */
    bool writeLines(const QString &path, const QStringList &lines) {
        QSaveFile sf(path);

        if (!sf.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        QTextStream out(&sf);
        out << lines.join(u'\n') << u'\n';

        return sf.commit();
    }

    /**
     * @brief Searches for all ChangeLog.txt files within the new database directory structure.
     * @return A QStringList containing the absolute paths to all found ChangeLog files.
     */
    QStringList fetchChangeLogs() {
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
    QStringList readConf() {
        QFile f(CONFIG_PATH);

        if (!f.exists()) {
            QStringList defaultTemplate = {
                OFFICIAL_HEADER, EMPTY,
                THIRD_HEADER, EMPTY,
                VERIFICATION_HEADER, EMPTY,
                EXCEPTION_HEADER, EMPTY,
                PRIORITY_HEADER, EMPTY,
                ADMIN_HEADER,
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
    bool writeConf(const QStringList &lines) {
        const QFileInfo fileInfo(CONFIG_PATH);

        if (const QDir dir = fileInfo.dir(); !dir.exists())
            (void) dir.mkpath(".");

        return writeLines(CONFIG_PATH, lines);
    }
}
