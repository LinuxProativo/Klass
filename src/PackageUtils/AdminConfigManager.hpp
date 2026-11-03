/**
 * @file AdminConfigManager.hpp
 * @brief Header for managing administrative, verification, and cache configurations in klass.conf.
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
    UpdateMimeDatabase,
    UpdateManDb,
    UpdateDesktopDatabase,
    UpdateGtkIconCache,
    UpdateGrub,
    UpdateLilo,
    UpdateElilo,
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

    [[nodiscard]] QString downloadCommand() const { return downloadCmd; }

    void setDownloadCommand(const QString &command) { downloadCmd = command; }

    [[nodiscard]] QString downloadExtraParams() const { return downloadCmdParams; }

    void setDownloadExtraParams(const QString &params) { downloadCmdParams = params; }

    [[nodiscard]] bool verifySignature() const { return checkSignature; }

    void setVerifySignature(const bool enabled) { checkSignature = enabled; }

    [[nodiscard]] bool verifyChecksum() const { return checkChecksum; }

    void setVerifyChecksum(const bool enabled) { checkChecksum = enabled; }

    [[nodiscard]] QString preferredChecksum() const { return preferChecksum; }

    void setPreferredChecksum(const QString &algorithm) { preferChecksum = algorithm; }

    [[nodiscard]] bool keepCache() const { return keepCacheEnabled; }

    void setKeepCache(const bool enabled) { keepCacheEnabled = enabled; }

    [[nodiscard]] bool postInstallTask(const PostInstallTask task) const { return postInstallTasks.value(task, false); }

    void setPostInstallTask(const PostInstallTask task, const bool enabled) { postInstallTasks[task] = enabled; }

    void load();

    [[nodiscard]] QString serializePayload() const;

    static bool savePayload(const QString &payload);

private:
    [[nodiscard]] static QString taskToKey(PostInstallTask task);

    bool checkSignature{true}, checkChecksum{true};
    bool keepCacheEnabled{false};
    QString downloadCmd{QStringLiteral("Native")};
    QString downloadCmdParams{};
    QString preferChecksum{QStringLiteral("MD5")};
    QMap<PostInstallTask, bool> postInstallTasks{};
};

#endif
