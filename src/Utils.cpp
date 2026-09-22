/**
 * @file Utils.cpp
 * @brief Implementation of general utility helper functions.
 */

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QTimer>

#include <Utils.hpp>
#include <zlib.h>

#include "SlackwareDefines.hpp"

/**
 * @brief Synchronously downloads data from a specified URL with Wget-like behavior.
 * @param url The target endpoint to fetch.
 * @param range Optional HTTP Range header (e.g., "bytes=0-255").
 * @param cancelCheck Optional callback executed periodically to query whether to abort the operation.
 * @return DownResult Containing execution status, HTTP status code, raw data payload, and network errors.
 */
DownloadResult Utils::download(const QUrl &url, const QByteArray &range, const std::function<bool()> &cancelCheck) {
    DownloadResult result;
    QNetworkRequest request(url);
    request.setMaximumRedirectsAllowed(20);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Wget/1.21.4");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Accept-Encoding", "identity");
    request.setRawHeader("Connection", "keep-alive");

    if (!range.isEmpty())
        request.setRawHeader("Range", range);

    QEventLoop loop;
    QNetworkAccessManager nam;
    QNetworkReply *reply = nam.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer timer;
    if (cancelCheck) {
        QObject::connect(&timer, &QTimer::timeout, [&] {
            if (cancelCheck()) {
                reply->abort();
                loop.quit();
            }
        });
        timer.start(100);
    }

    loop.exec();
    timer.stop();

    result.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.error = reply->error();

    if (result.error == QNetworkReply::NoError && result.statusCode >= 200 && result.statusCode < 300) {
        result.success = true;
        result.data = reply->readAll();
    }

    reply->deleteLater();
    return result;
}

/**
 * @brief Collapses runs of consecutive spaces into a single space and drops one leading space.
 * @param raw View over the raw text to normalize (not modified; source must outlive the call).
 * @return A newly built string with normalized internal whitespace.
 */
QString Utils::normalizeSpaces(QStringView raw) {
    if (raw.startsWith(u' '))
        raw = raw.mid(1);

    QString normalized;
    normalized.reserve(raw.size());
    bool lastWasSpace = false;

    for (const QChar ch: raw) {
        if (ch == u' ') {
            if (!lastWasSpace) {
                normalized.append(ch);
                lastWasSpace = true;
            }
        } else {
            normalized.append(ch);
            lastWasSpace = false;
        }
    }
    return normalized;
}

/**
 * @brief Decompresses a targeted Gzip archive data stream directly into a memory buffer.
 * @param filePath The exact source target address pointing to the archived tracking manifest on disk.
 * @return The fully inflated plain text document string, or a null-initialized fallback container.
 */
QString Utils::readGzipFile(const QString &filePath) {
    const auto file = gzopen(filePath.toLocal8Bit().constData(), "rb");
    if (!file)
        return {};

    QByteArray uncompressedData;
    char buffer[16384];
    int bytesRead = 0;

    if (const QFile f(filePath); f.exists())
        uncompressedData.reserve(static_cast<int>(f.size() * 5));

    while ((bytesRead = gzread(file, buffer, sizeof(buffer))) > 0)
        uncompressedData.append(buffer, bytesRead);

    gzclose(file);
    return QString::fromUtf8(uncompressedData);
}

/**
 * @brief Enables or disables the XDG autostart entry for the application.
 * @param enable True to create the autostart entry, false to remove it.
 */
void Utils::setAutostart(const bool enable) {
    const QDir autostartDir(AUTOSTART_CONFIG);

    if (!autostartDir.exists()) {
        if (!autostartDir.mkpath(QStringLiteral(".")))
            return;
    }

    const QString desktopFilePath = autostartDir.absoluteFilePath(DESKTOP_FILE);

    if (enable) {
        QFile file(desktopFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

        QTextStream out(&file);
        out << "[Desktop Entry]\n"
            << "Type=Application\n"
            << "Name=Klass Package Manager\n"
            << "Comment=Starts the Klass Package Manager in the background\n"
            << "Exec=klass --autostart\n"
            << "Icon=klass\n"
            << "Terminal=false\n"
            << "Categories=System;Utility;\n"
            << "X-GNOME-Autostart-enabled=true\n"
            << "X-KDE-autostart-after=panel\n";

        file.close();
        return;
    }

    if (QFile::exists(desktopFilePath))
        QFile::remove(desktopFilePath);
}
