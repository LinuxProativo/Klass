/**
 * @file AdminConfigManager.cpp
 * @brief Implementation of AdminConfigManager for persistent settings in /etc/klass/klass.conf.
 */

#include <AdminConfigManager.hpp>
#include <RepoManager.hpp>
#include <SlackwareDefines.hpp>

inline const auto SECTION_HEADER = QStringLiteral("# Administrative Actions");

/**
 * @brief Constructs an AdminConfigManager instance and loads existing settings.
 */
AdminConfigManager::AdminConfigManager() {
    m_postInstallTasks[PostInstallTask::Ldconfig] = true;
    m_postInstallTasks[PostInstallTask::UpdateManDb] = true;
    m_postInstallTasks[PostInstallTask::UpdateGtkIconCache] = false;
    m_postInstallTasks[PostInstallTask::UpdateDesktopDatabase] = false;
    m_postInstallTasks[PostInstallTask::UpdateGrub] = false;
    m_postInstallTasks[PostInstallTask::UpdateLilo] = false;
    m_postInstallTasks[PostInstallTask::GenerateInitrd] = false;
    m_postInstallTasks[PostInstallTask::ReinstallVBoxModules] = false;

    load();
}

/**
 * @brief Checks if package signature verification is active.
 * @return True if verification is enabled, false otherwise.
 */
bool AdminConfigManager::verifySignature() const {
    return m_verifySignature;
}

/**
 * @brief Sets the signature verification state.
 * @param enabled True to enable signature checks, false to disable.
 */
void AdminConfigManager::setVerifySignature(const bool enabled) {
    m_verifySignature = enabled;
}

/**
 * @brief Checks if package checksum verification is active.
 * @return True if checksum checks are enabled, false otherwise.
 */
bool AdminConfigManager::verifyChecksum() const {
    return m_verifyChecksum;
}

/**
 * @brief Sets the checksum verification state.
 * @param enabled True to enable checksum checks, false to disable.
 */
void AdminConfigManager::setVerifyChecksum(const bool enabled) {
    m_verifyChecksum = enabled;
}

/**
 * @brief Returns the preferred checksum algorithm.
 * @return The algorithm identifier string (e.g. "MD5").
 */
QString AdminConfigManager::preferredChecksum() const {
    return m_preferredChecksum;
}

/**
 * @brief Sets the preferred checksum algorithm.
 * @param algorithm The algorithm identifier string.
 */
void AdminConfigManager::setPreferredChecksum(const QString &algorithm) {
    m_preferredChecksum = algorithm;
}

/**
 * @brief Checks if a specific post-installation task is enabled.
 * @param task The identifier of the maintenance task to verify.
 * @return True if the task is scheduled to run, false otherwise.
 */
bool AdminConfigManager::postInstallTask(const PostInstallTask task) const {
    return m_postInstallTasks.value(task, false);
}

/**
 * @brief Updates the execution flag for a post-installation task.
 * @param task The maintenance task to update.
 * @param enabled True to enable execution, false to disable.
 */
void AdminConfigManager::setPostInstallTask(const PostInstallTask task, const bool enabled) {
    m_postInstallTasks[task] = enabled;
}

/**
 * @brief Maps a post-installation task enum value to its configuration key string.
 * @param task The task enumeration value.
 * @return The corresponding configuration key name.
 */
QString AdminConfigManager::taskToKey(const PostInstallTask task) {
    switch (task) {
        case PostInstallTask::Ldconfig:
            return QStringLiteral("TASK_LDCONFIG");
        case PostInstallTask::UpdateManDb:
            return QStringLiteral("TASK_UPDATE_MANDB");
        case PostInstallTask::UpdateGtkIconCache:
            return QStringLiteral("TASK_UPDATE_GTK_ICON_CACHE");
        case PostInstallTask::UpdateDesktopDatabase:
            return QStringLiteral("TASK_UPDATE_DESKTOP_DATABASE");
        case PostInstallTask::UpdateGrub:
            return QStringLiteral("TASK_UPDATE_GRUB");
        case PostInstallTask::UpdateLilo:
            return QStringLiteral("TASK_UPDATE_LILO");
        case PostInstallTask::GenerateInitrd:
            return QStringLiteral("TASK_GENERATE_INITRD");
        case PostInstallTask::ReinstallVBoxModules:
            return QStringLiteral("TASK_REINSTALL_VBOX_MODULES");
    }
    return {};
}

/**
 * @brief Reads administrative parameters from /etc/klass/klass.conf.
 */
void AdminConfigManager::load() {
    const QStringList lines = RepoManager::readConf();
    bool inSection = false;

    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();

        if (trimmed.startsWith(u'#')) {
            inSection = (trimmed.compare(SECTION_HEADER, Qt::CaseInsensitive) == 0);
            continue;
        }

        if (!inSection || trimmed.isEmpty())
            continue;

        const int eqIdx = static_cast<int>(trimmed.indexOf(u'='));
        if (eqIdx == -1)
            continue;

        const QString key = trimmed.left(eqIdx).trimmed().toUpper();
        const QString val = trimmed.mid(eqIdx + 1).trimmed();

        if (key == QStringLiteral("VERIFY_SIGNATURE")) {
            m_verifySignature = (val.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 || val == QStringLiteral("1"));
        } else if (key == QStringLiteral("VERIFY_CHECKSUM")) {
            m_verifyChecksum = (val.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 || val == QStringLiteral("1"));
        } else if (key == QStringLiteral("PREFERRED_CHECKSUM")) {
            if (!val.isEmpty())
                m_preferredChecksum = val;
        } else {
            for (auto it = m_postInstallTasks.begin(); it != m_postInstallTasks.end(); ++it) {
                if (key == taskToKey(it.key())) {
                    it.value() = (val.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0 || val == QStringLiteral("1"));
                    break;
                }
            }
        }
    }
}

/**
 * @brief Serializes the current settings into a semicolon-delimited key-value string.
 * @return A serialized representation for IPC transmission to the helper.
 */
QString AdminConfigManager::serializePayload() const {
    QStringList items;

    items << QStringLiteral("VERIFY_SIGNATURE=") + (m_verifySignature ? QStringLiteral("true") : QStringLiteral("false"));
    items << QStringLiteral("VERIFY_CHECKSUM=") + (m_verifyChecksum ? QStringLiteral("true") : QStringLiteral("false"));
    items << QStringLiteral("PREFERRED_CHECKSUM=") + m_preferredChecksum;

    for (auto it = m_postInstallTasks.cbegin(); it != m_postInstallTasks.cend(); ++it) {
        items << taskToKey(it.key()) + QStringLiteral("=") + (it.value() ? QStringLiteral("true") : QStringLiteral("false"));
    }

    return items.join(u';');
}

/**
 * @brief Writes the configuration block into /etc/klass/klass.conf preserving other sections.
 * @param payload The serialized key-value pairs to write.
 * @return True on successful write and commit, false otherwise.
 */
bool AdminConfigManager::savePayload(const QString &payload) {
    QStringList lines = RepoManager::readConf();

    int sectionStart = -1;
    int sectionEnd = -1;

    for (int i = 0; i < lines.size(); ++i) {
        const QString trimmed = lines.at(i).trimmed();
        if (trimmed.compare(SECTION_HEADER, Qt::CaseInsensitive) == 0) {
            sectionStart = i;
            sectionEnd = lines.size();
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

    QStringList adminBlock;
    adminBlock << SECTION_HEADER;

    for (const QString &pair : payload.split(u';', Qt::SkipEmptyParts))
        adminBlock << pair.trimmed();

    adminBlock << QStringLiteral("");

    lines.append(adminBlock);
    return RepoManager::writeConf(lines);
}
