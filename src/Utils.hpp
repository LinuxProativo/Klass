/**
 * @file Utils.hpp
 * @brief Header for general network utilities and helper functions.
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <QByteArray>
#include <QList>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringView>
#include <QUrl>

#include <functional>

#include <SlackwareDefines.hpp>

// Declaração de Estrutura
struct PendingPkg;

/**
 * @struct DownloadResult
 * @brief Represents the result status, HTTP response metadata, and payload of a download operation.
 */
struct DownloadResult {
    bool success{false};
    int statusCode{0};
    QByteArray data{};
    QString computedMd5{};
    QNetworkReply::NetworkError error{QNetworkReply::NoError};
};

/**
 * @namespace Utils
 * @brief Utility class providing static network and application helper methods.
 */
namespace Utils {
    DownloadResult download(const QUrl &url, const QString &destinationPath = {},
                            const QByteArray &range = {}, const std::function<bool()> &cancelCheck = nullptr);

    QString readGzipFile(const QString &filePath);

    void setAutostart(bool enable);

    QString extractBuildTag(QStringView version);

    QString serializePendingList(const QList<PendingPkg> &list);

    void releaseFreeMemory();

    inline bool isOfficialRepo(const QString &r) {
        return r == SLACK_OFICIAL || r == SLACK_PATCHES || r == SLACK_EXTRA ||
               r == SLACK_TESTING || r == SLACK_PASTURE;
    }
};

#endif
