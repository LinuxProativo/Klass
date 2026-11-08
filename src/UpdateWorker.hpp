/**
 * @file UpdateWorker.hpp
 * @brief Header for asynchronous background update checking thread.
 */

#ifndef UPDATEWORKER_HPP
#define UPDATEWORKER_HPP

#include <QThread>
#include <atomic>

/**
 * @class UpdateWorker
 * @brief Dedicated background thread for periodic package database update checking.
 */
class UpdateWorker final : public QThread {
    Q_OBJECT

public:
    explicit UpdateWorker(QObject *parent = nullptr) : QThread(parent) {}

    ~UpdateWorker() override;

    void resetState() { updatePending.store(false, std::memory_order_relaxed); }

signals:
    void updateAvailable();

    void checkStarted();

    void checkFinished();

protected:
    void run() override;

private:
    void check();

    std::atomic<bool> updatePending{false};
};

#endif
