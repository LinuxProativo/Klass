/**
 * @file RepoUtils.cpp
 * @brief Implementation of static internal package and repository utility functions.
 */

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>

#include <RepoUtils.hpp>
#include <SlackwareDefines.hpp>

namespace RepoUtils {
    /**
     * @brief Returns the machine architecture as used in mirror paths.
     * @return "x86_64", "x86", or an empty QString when undetermined.
     */
    QString detectArch() {
        const QString arch = QSysInfo::buildCpuArchitecture();
        return (arch == u"i686" || arch == u"i586" || arch == u"i486" || arch == u"i386") ? QStringLiteral("x86") : arch;
    }

    /**
     * @brief Collapses runs of consecutive spaces into a single space and drops leading whitespace.
     * @param raw View over the raw text to normalize.
     * @return A newly built string with normalized internal whitespace.
     */
    QString normalizeSpaces(const QStringView raw) {
        if (raw.startsWith(u' '))
            return normalizeSpaces(raw.mid(1));

        QString normalized;
        normalized.reserve(raw.size());
        bool lastWasSpace = false;

        for (const QChar ch : raw) {
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
     * @brief Parses a boolean string representation commonly found in configuration files.
     * @param value Text view containing "true", "1", "false", "0", etc.
     * @param defaultValue Fallback value if undetermined.
     * @return Resolved boolean value.
     */
    bool parseBool(const QStringView value, const bool defaultValue) {
        const QString trimmed = value.trimmed().toString();
        if (trimmed.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0 ||
            trimmed == QLatin1String("1") ||
            trimmed.compare(QLatin1String("yes"), Qt::CaseInsensitive) == 0) {
            return true;
        }
        if (trimmed.compare(QLatin1String("false"), Qt::CaseInsensitive) == 0 ||
            trimmed == QLatin1String("0") ||
            trimmed.compare(QLatin1String("no"), Qt::CaseInsensitive) == 0) {
            return false;
        }
        return defaultValue;
    }

    /**
     * @brief Checks if a given filename matches standard Slackware package archive extensions.
     * @param fileName File name or path view to inspect.
     * @return True if ending in .txz, .tgz, .tbz, or .tlz.
     */
    bool isPackageArchive(const QStringView fileName) {
        return fileName.endsWith(u".txz") || fileName.endsWith(u".tgz") ||
               fileName.endsWith(u".tbz") || fileName.endsWith(u".tlz");
    }

    /**
     * @brief Appends and extracts description metadata from a Slackware description line.
     * @param rawLine Raw text line from the package manifest or installed log.
     * @param prefix Package prefix (e.g., "packagename:").
     * @param outShortDesc Reference to the short summary description to populate.
     * @param outDetailedDesc Reference to the multi-line detailed description buffer.
     */
    void appendPackageDescription(const QStringView rawLine, const QStringView prefix,
                                  QString &outShortDesc, QString &outDetailedDesc) {
        QStringView content = rawLine;
        if (!prefix.isEmpty() && content.startsWith(prefix, Qt::CaseInsensitive))
            content = content.mid(prefix.length());

        const QString contentStr = normalizeSpaces(content);
        const QString trimmed = contentStr.trimmed();

        if (outShortDesc.isEmpty() && !trimmed.isEmpty()) {
            const int openParen = static_cast<int>(trimmed.indexOf(u'('));
            const int closeParen = static_cast<int>(trimmed.lastIndexOf(u')'));
            if (openParen != -1 && closeParen > openParen)
                outShortDesc = trimmed.mid(openParen + 1, closeParen - openParen - 1);
            else
                outShortDesc = trimmed;
        }

        if (!contentStr.isEmpty()) {
            outDetailedDesc += contentStr + u'\n';
        } else if (!outDetailedDesc.isEmpty() && !outDetailedDesc.endsWith(u"\n\n")) {
            outDetailedDesc += u'\n';
        }
    }

    /**
     * @brief Atomically writes @p lines to @p path using QSaveFile.
     * @param path Destination filesystem path.
     * @param lines Content lines to write.
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
     * @brief Searches for all ChangeLog.txt files within the database directory structure.
     * @return A QStringList containing the absolute paths to all found ChangeLog files.
     */
    QStringList fetchChangeLogs() {
        const QDir rootDir(KLASS_DATABASE);
        const QStringList repos = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QStringList foundFiles;

        for (const QString &repo : repos) {
            const QDir repoDir(KLASS_DATABASE + repo);
            for (const QString &file : repoDir.entryList({QStringLiteral("ChangeLog.txt")}, QDir::Files, QDir::Name))
                foundFiles << repoDir.absoluteFilePath(file);
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
            const QStringList defaultTemplate = {
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
            (void) dir.mkpath(QStringLiteral("."));

        return writeLines(CONFIG_PATH, lines);
    }

    /**
     * @brief Retrieves the trimmed value of a configuration line beginning with a specific key prefix.
     * @param keyPrefix Prefix identifying the target key (e.g. "OFFICIAL_MIRROR=").
     * @return Value substring or empty string if not found.
     */
    QString readConfigValue(const QString &keyPrefix) {
        for (const QString &line : readConf()) {
            if (const QString trimmed = line.trimmed(); trimmed.startsWith(keyPrefix))
                return trimmed.mid(keyPrefix.length()).trimmed();
        }
        return {};
    }

    /**
     * @brief Updates an existing configuration line or inserts it under the designated section header.
     * @param sectionHeader Header line where new entries should be anchored (e.g. OFFICIAL_HEADER).
     * @param keyPrefix Key prefix identifying an existing entry to replace.
     * @param newLine Full replacement or new configuration line to write.
     * @return True if committed successfully.
     */
    bool setConfigValue(const QString &sectionHeader, const QString &keyPrefix, const QString &newLine) {
        QStringList lines = readConf();
        bool updated = false;

        for (QString &line : lines) {
            if (line.trimmed().startsWith(keyPrefix)) {
                line = newLine;
                updated = true;
                break;
            }
        }

        if (!updated) {
            if (const int idx = static_cast<int>(lines.indexOf(sectionHeader)); idx != -1) {
                lines.insert(idx + 1, newLine);
            } else {
                lines.append(newLine);
            }
        }

        return writeConf(lines);
    }

    /**
     * @brief Removes any configuration line starting with the given prefix.
     * @param keyPrefix Prefix identifying entries to delete.
     * @return True on success.
     */
    bool removeConfigValue(const QString &keyPrefix) {
        const QStringList lines = readConf();
        QStringList updated;
        bool removed = false;

        for (const QString &line : lines) {
            if (line.trimmed().startsWith(keyPrefix)) {
                removed = true;
                continue;
            }
            updated.append(line);
        }

        return removed ? writeConf(updated) : true;
    }

    /**
     * @brief Removes exact matching configuration lines.
     * @param targets Set of exact trimmed line strings to remove.
     * @return True on success.
     */
    bool removeConfigLines(const QSet<QString> &targets) {
        const QStringList currentLines = readConf();
        QStringList updatedLines;
        updatedLines.reserve(currentLines.size());

        for (const QString &line : currentLines) {
            if (!targets.contains(line.trimmed()))
                updatedLines.append(line);
        }

        return writeConf(updatedLines);
    }

    /**
     * @brief Appends a configuration line under the specified section header if not already present.
     * @param sectionHeader Section header to place the line under.
     * @param newLine The configuration line to append.
     * @return True on success.
     */
    bool appendConfigLine(const QString &sectionHeader, const QString &newLine) {
        QStringList lines = readConf();
        for (const QString &line : lines) {
            if (line.trimmed() == newLine)
                return true;
        }

        if (const int idx = static_cast<int>(lines.indexOf(sectionHeader)); idx != -1) {
            lines.insert(idx + 1, newLine);
        } else {
            lines.append(newLine);
        }

        return writeConf(lines);
    }

    /**
     * @brief Replaces an exact existing configuration line with a new line.
     * @param oldLine Exact line currently in configuration.
     * @param newLine New line to substitute.
     * @return True on success or if old line was absent.
     */
    bool replaceConfigLine(const QString &oldLine, const QString &newLine) {
        QStringList lines = readConf();
        bool found = false;

        for (QString &line : lines) {
            if (line.trimmed() == oldLine) {
                line = newLine;
                found = true;
                break;
            }
        }

        return found ? writeConf(lines) : true;
    }

    /**
     * @brief Replaces all content under a given section header until the next header or EOF.
     * @param sectionHeader Target section header identifier.
     * @param newSectionLines Sequence of lines to place inside the section.
     * @return True on success.
     */
    bool replaceConfigSection(const QString &sectionHeader, const QStringList &newSectionLines) {
        QStringList lines = readConf();
        int sectionStart = -1;
        int sectionEnd = static_cast<int>(lines.size());

        for (int i = 0; i < lines.size(); ++i) {
            if (lines.at(i).trimmed().compare(sectionHeader, Qt::CaseInsensitive) == 0) {
                sectionStart = i;
                for (int j = i + 1; j < lines.size(); ++j) {
                    if (lines.at(j).trimmed().startsWith(u'#')) {
                        sectionEnd = j;
                        break;
                    }
                }
                break;
            }
        }

        if (sectionStart != -1) {
            for (int i = sectionEnd - 1; i >= sectionStart; --i)
                lines.removeAt(i);
        }

        QStringList block;
        block << sectionHeader;
        block.append(newSectionLines);
        block << QStringLiteral("");

        lines.append(block);
        return writeConf(lines);
    }
}
