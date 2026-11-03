/**
 * @file RepoVerificationManager.hpp
 * @brief Header for managing repository verification settings in klass.conf.
 */

#ifndef REPOVERIFICATIONMANAGER_HPP
#define REPOVERIFICATIONMANAGER_HPP

#include <QList>

/**
 * @struct RepoVerificationEntry
 * @brief Holds verification settings for a third-party repository.
 */
struct RepoVerificationEntry {
    QString repo{};
    bool verifySig{true};
    bool verifyChecksumSig{true};
    bool verifyChecksum{true};
    QString preferredChecksum{QStringLiteral("MD5")};

    [[nodiscard]] QString toConfigLine() const {
        return QStringLiteral("VERIFICATION['%1']=(%2,%3,%4,%5)")
                .arg(repo,
                     verifySig ? QStringLiteral("true") : QStringLiteral("false"),
                     verifyChecksumSig ? QStringLiteral("true") : QStringLiteral("false"),
                     verifyChecksum ? QStringLiteral("true") : QStringLiteral("false"),
                     preferredChecksum);
    }
};

/**
 * @class RepoVerificationManager
 * @brief Reads and writes third-party repository verification rules to /etc/klass/klass.conf.
 */
class RepoVerificationManager {
public:
    static QList<RepoVerificationEntry> loadVerifications();

    static RepoVerificationEntry getVerification(const QString &repoName);

    static bool saveVerification(const RepoVerificationEntry &entry);

    static bool removeVerification(const QString &repoName);
};

#endif
