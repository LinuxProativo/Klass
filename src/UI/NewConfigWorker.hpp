/**
 * @file NewConfigWorker.hpp
 * @brief Header for the asynchronous diff calculation worker.
 */

#ifndef NEWCONFIGWORKER_HPP
#define NEWCONFIGWORKER_HPP

#include <QObject>
#include <QString>
#include <QList>

#include <DiffEngine.hpp>

/**
 * @class NewConfigWorker
 * @brief Dedicated background worker for reading files and generating diffs without blocking the GUI.
 */
class NewConfigWorker final : public QObject {
    Q_OBJECT

public:
    explicit NewConfigWorker(QObject *parent = nullptr);

    ~NewConfigWorker() override = default;

public slots:
    void processDiff(quint64 requestId, const QString &oldPath, const QString &newPath);

signals:
    void diffFinished(quint64 requestId, const QString &oldPath, const QString &newPath,
                      const QList<DiffEngine::DiffLine> &diffLines, bool oldExists);
};

#endif
