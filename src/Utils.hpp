/**
 * @file Utils.hpp
 * @brief Header for general network utilities and helper functions.
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <QByteArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <functional>

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
};

#endif
