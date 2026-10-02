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

    [[nodiscard]] bool verifySignature() const { return checkSignature; }

    void setVerifySignature(const bool enabled) { checkSignature = enabled; }

    [[nodiscard]] bool verifyChecksum() const { return checkChecksum; }

    void setVerifyChecksum(const bool enabled) { checkChecksum = enabled; }

    [[nodiscard]] QString preferredChecksum() const { return preferChecksum; }

    void setPreferredChecksum(const QString &algorithm) { preferChecksum = algorithm; }

    [[nodiscard]] bool postInstallTask(const PostInstallTask task) const { return postInstallTasks.value(task, false); }

    void setPostInstallTask(const PostInstallTask task, const bool enabled) { postInstallTasks[task] = enabled; }

    void load();

    [[nodiscard]] QString serializePayload() const;

    static bool savePayload(const QString &payload);

private:
    [[nodiscard]] static QString taskToKey(PostInstallTask task);

    bool checkSignature{true}, checkChecksum{true};
    QString preferChecksum{QStringLiteral("MD5")};
    QMap<PostInstallTask, bool> postInstallTasks{};
};

#endif
