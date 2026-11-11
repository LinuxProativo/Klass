/**
 * @file NewConfigWorker.cpp
 * @brief Implementation of the asynchronous diff computation worker.
 */

#include <QFile>
#include <NewConfigManager.hpp>
#include <NewConfigWorker.hpp>

/**
 * @brief Constructs the NewConfigWorker instance.
 * @param parent Optional parent QObject.
 */
NewConfigWorker::NewConfigWorker(QObject *parent) : QObject(parent) {}

/**
 * @brief Reads file contents from disk and computes the line-by-line diff.
 * @param requestId Monotonic request identifier used to discard outdated queries.
 * @param oldPath Absolute path to the existing configuration file.
 * @param newPath Absolute path to the incoming .new configuration file.
 */
void NewConfigWorker::processDiff(const quint64 requestId, const QString &oldPath, const QString &newPath) {
    const bool oldExists = QFile::exists(oldPath);
    const QString oldContent = NewConfigManager::readFileContent(oldPath);
    const QString newContent = NewConfigManager::readFileContent(newPath);

    QList<DiffEngine::DiffLine> diff;
    if (oldExists) {
        diff = DiffEngine::computeDiff(oldContent, newContent);
    } else {
        diff = DiffEngine::computeDiff(QString{}, newContent);
    }

    emit diffFinished(requestId, oldPath, newPath, diff, oldExists);
}
