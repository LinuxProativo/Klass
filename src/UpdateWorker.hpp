/**
 * @file UpdateWorker.hpp
 * @brief Header for run UpdateWorker in thread.
 */

#ifndef UPDATEWORKER_HPP
#define UPDATEWORKER_HPP

#include <QTimer>

/**
 * @class UpdateWorker
 * @brief Periodically checks whether the package database requires an update in a dedicated thread.
 */
class UpdateWorker final : public QObject {
    Q_OBJECT

public:
    UpdateWorker() = default;

    void start();

signals:
    void updateAvailable();

private:
    void check();
};

#endif
