/**
 * @file RepoVerificationManager.cpp
 * @brief Implementation of RepoVerificationManager.
 */

#include <QRegularExpression>

#include <RepoUtils.hpp>
#include <RepoVerificationManager.hpp>

static const QRegularExpression VERIF_REGEX(R"(^VERIFICATION\['([^']+)'\]\s*=\s*\(([^,]+),([^,]+),([^,]+),([^)]+)\)$)");

/**
 * @brief Loads all verification entries from klass.conf.
 * @return List of parsed verification entries.
 */
QList<RepoVerificationEntry> RepoVerificationManager::loadVerifications() {
    const QStringList lines = RepoUtils::readConf();
    QList<RepoVerificationEntry> result;

    for (const QString &line : lines) {
        if (const auto m = VERIF_REGEX.match(line.trimmed()); m.hasMatch()) {
            RepoVerificationEntry entry;
            entry.repo = m.captured(1).trimmed();
            entry.verifySig = RepoUtils::parseBool(m.captured(2), true);
            entry.verifyChecksumSig = RepoUtils::parseBool(m.captured(3), true);
            entry.verifyChecksum = RepoUtils::parseBool(m.captured(4), true);
            entry.preferredChecksum = m.captured(5).trimmed();
            result.append(entry);
        }
    }
    return result;
}

/**
 * @brief Retrieves verification settings for a specific repository. Defaults to all true if not set.
 * @param repoName Target repository name.
 * @return Verification entry configured or default values.
 */
RepoVerificationEntry RepoVerificationManager::getVerification(const QString &repoName) {
    for (const auto &entry : loadVerifications()) {
        if (entry.repo == repoName)
            return entry;
    }
    return RepoVerificationEntry{repoName, true, true, true, QStringLiteral("MD5")};
}

/**
 * @brief Saves or updates a verification rule under '# Verifications'.
 * @param entry The configuration entry to persist.
 * @return True on success.
 */
bool RepoVerificationManager::saveVerification(const RepoVerificationEntry &entry) {
    const QString targetPrefix = QStringLiteral("VERIFICATION['") + entry.repo + QStringLiteral("']");
    return RepoUtils::setConfigValue(VERIFICATION_HEADER, targetPrefix, entry.toConfigLine());
}

/**
 * @brief Removes a verification rule when a repository is deleted.
 * @param repoName The repository name to remove.
 * @return True on success.
 */
bool RepoVerificationManager::removeVerification(const QString &repoName) {
    const QString targetPrefix = QStringLiteral("VERIFICATION['") + repoName + QStringLiteral("']");
    return RepoUtils::removeConfigValue(targetPrefix);
}
