/**
 * @file AdminConfigManager.cpp
 * @brief Implementation of AdminConfigManager for persistent settings in /etc/klass/klass.conf.
 */

#include <AdminConfigManager.hpp>
#include <RepoManager.hpp>
#include <SlackwareDefines.hpp>

/**
 * @brief Constructs an AdminConfigManager instance and loads existing settings.
 */
AdminConfigManager::AdminConfigManager() {
    postInstallTasks[PostInstallTask::Ldconfig] = true;
    postInstallTasks[PostInstallTask::UpdateManDb] = true;
    postInstallTasks[PostInstallTask::UpdateGtkIconCache] = false;
    postInstallTasks[PostInstallTask::UpdateDesktopDatabase] = false;
    postInstallTasks[PostInstallTask::UpdateGrub] = false;
    postInstallTasks[PostInstallTask::UpdateLilo] = false;
    postInstallTasks[PostInstallTask::GenerateInitrd] = false;
    postInstallTasks[PostInstallTask::ReinstallVBoxModules] = false;

    load();
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

    for (const QString &line: lines) {
        const QString trimmed = line.trimmed();

        if (trimmed.startsWith(u'#')) {
            inSection = (trimmed.compare(ADMIN_HEADER, Qt::CaseInsensitive) == 0);
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
            checkSignature = val.compare(QStringLiteral("true")) == 0 || val == QStringLiteral("1");
        } else if (key == QStringLiteral("VERIFY_CHECKSUM")) {
            checkChecksum = val.compare(QStringLiteral("true")) == 0 || val == QStringLiteral("1");
        } else if (key == QStringLiteral("PREFERRED_CHECKSUM")) {
            if (!val.isEmpty())
                preferChecksum = val;
        } else {
            for (auto it = postInstallTasks.begin(); it != postInstallTasks.end(); ++it) {
                if (key == taskToKey(it.key())) {
                    it.value() = val.compare(QStringLiteral("true")) == 0 || val == QStringLiteral("1");
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

    items << QStringLiteral("VERIFY_SIGNATURE=") + (checkSignature ? QStringLiteral("true") : QStringLiteral("false"));
    items << QStringLiteral("VERIFY_CHECKSUM=") + (checkChecksum ? QStringLiteral("true") : QStringLiteral("false"));
    items << QStringLiteral("PREFERRED_CHECKSUM=") + preferChecksum;

    for (auto it = postInstallTasks.cbegin(); it != postInstallTasks.cend(); ++it)
        items << taskToKey(it.key()) + QStringLiteral("=") + (it.value()
                                                                  ? QStringLiteral("true")
                                                                  : QStringLiteral("false"));
    return items.join(u';');
}

/**
 * @brief Writes the configuration block into /etc/klass/klass.conf preserving other sections.
 * @param payload The serialized key-value pairs to write.
 * @return True on successful write and commit, false otherwise.
 */
bool AdminConfigManager::savePayload(const QString &payload) {
    QStringList lines = RepoManager::readConf();
    int sectionStart = -1, sectionEnd = 0;

    for (int i = 0; i < lines.size(); ++i) {
        if (lines.at(i).trimmed().compare(ADMIN_HEADER, Qt::CaseInsensitive) == 0) {
            sectionStart = i;
            sectionEnd = static_cast<int>(lines.size());
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
    adminBlock << ADMIN_HEADER;

    for (const QString &pair: payload.split(u';', Qt::SkipEmptyParts))
        adminBlock << pair.trimmed();

    adminBlock << QStringLiteral("");

    lines.append(adminBlock);
    return RepoManager::writeConf(lines);
}
