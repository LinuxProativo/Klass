/**
* @file AdminConfigManager.hpp
 * @brief Header for managing administrative and verification configurations in klass.conf.
 */

#ifndef ADMINCONFIGMANAGER_HPP
#define ADMINCONFIGMANAGER_HPP

#include <QMap>
#include <QString>

/**
 * @enum PostInstallTask
 * @brief Identifiers for post-installation administrative maintenance tasks.
 */
enum class PostInstallTask {
    Ldconfig,
    UpdateManDb,
    UpdateGtkIconCache,
    UpdateDesktopDatabase,
    UpdateGrub,
    UpdateLilo,
    GenerateInitrd,
    ReinstallVBoxModules
};

/**
 * @class AdminConfigManager
 * @brief Handles reading and writing administrative configurations to /etc/klass/klass.conf.
 */
class AdminConfigManager {
public:
    explicit AdminConfigManager();

    [[nodiscard]] bool verifySignature() const;

    void setVerifySignature(bool enabled);

    [[nodiscard]] bool verifyChecksum() const;

    void setVerifyChecksum(bool enabled);

    [[nodiscard]] QString preferredChecksum() const;

    void setPreferredChecksum(const QString &algorithm);

    [[nodiscard]] bool postInstallTask(PostInstallTask task) const;

    void setPostInstallTask(PostInstallTask task, bool enabled);

    void load();

    [[nodiscard]] QString serializePayload() const;

    static bool savePayload(const QString &payload);

private:
    [[nodiscard]] static QString taskToKey(PostInstallTask task);

    bool m_verifySignature{true};
    bool m_verifyChecksum{true};
    QString m_preferredChecksum{QStringLiteral("MD5")};
    QMap<PostInstallTask, bool> m_postInstallTasks{};
};

#endif
