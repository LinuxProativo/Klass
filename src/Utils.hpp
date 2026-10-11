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

// Declaração de Estrutura
struct PendingPkg;

/**
 * @struct DownloadResult
 * @brief Represents the result status, HTTP response metadata, and payload of a download operation.
 */
struct DownloadResult {
    bool success{false};
    int statusCode{0};
    QByteArray data;
    QNetworkReply::NetworkError error{QNetworkReply::NoError};
};

/**
 * @class Utils
 * @brief Utility class providing static network and application helper methods.
 */
class Utils {
public:
    static DownloadResult download(const QUrl &url, const QByteArray &range = {},
                                   const std::function<bool()> &cancelCheck = nullptr);

    static QString normalizeSpaces(QStringView raw);

    static QString readGzipFile(const QString &filePath);

    static void setAutostart(bool enable);

    static QString extractBuildTag(QStringView version);

    static bool isVersionNewer(const QString &v1, const QString &v2);

    static QString serializePendingList(const QList<PendingPkg> &list);
};

#endif
