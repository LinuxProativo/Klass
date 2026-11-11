/**
 * @file NewConfigWorker.cpp
 * @brief Implementation of the asynchronous diff computation worker.
 */

#include <QFile>
#include <QTextStream>

#include <Debug.hpp>
#include <NewConfigWorker.hpp>

/**
 * @brief Constructs the NewConfigWorker instance.
 * @param parent Optional parent QObject.
 */
NewConfigWorker::NewConfigWorker(QObject *parent) : QObject(parent) {}

/**
 * @brief Safely reads file content from disk and identifies permission issues.
 * @param path Absolute filesystem path to the target file.
 * @param denied Output boolean reference set to true if read fails due to permission errors.
 * @return Entire text content of the file, or an empty string if reading fails.
 */
QString NewConfigWorker::readFileSafe(const QString &path, bool &denied) {
    if (path.isEmpty())
        return {};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Debug::msg("Error to open file: " + file.errorString() + ". Code: " + QString::number(file.error()),
                   "NewConfigWorker", {DColor::LightRed});

        if (file.error() == QFileDevice::PermissionsError || file.error() == QFileDevice::OpenError)
            denied = true;

        return {};
    }

    QTextStream in(&file);
    return in.readAll();
}

/**
 * @brief Reads file contents from disk, detects permission errors, and computes line diff.
 * @param requestId Monotonic request identifier used to discard outdated queries.
 * @param oldPath Absolute path to the existing configuration file.
 * @param newPath Absolute path to the incoming .new configuration file.
 */
void NewConfigWorker::processDiff(const quint64 requestId, const QString &oldPath, const QString &newPath) {
    bool permissionDenied = false;

    const bool oldExists = QFile::exists(oldPath);
    const QString oldContent = readFileSafe(oldPath, permissionDenied);
    const QString newContent = readFileSafe(newPath, permissionDenied);

    QList<DiffEngine::DiffLine> diff;
    if (!permissionDenied) {
        if (oldExists) {
            diff = DiffEngine::computeDiff(oldContent, newContent);
        } else {
            diff = DiffEngine::computeDiff(QString{}, newContent);
        }
    }

    emit diffFinished(requestId, oldPath, newPath, diff, oldExists, permissionDenied);
}
