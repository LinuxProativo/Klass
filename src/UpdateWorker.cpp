/**
 * @file UpdateWorker.cpp
 * @brief Implementation of the asynchronous background update checking thread.
 */

#include <QTimer>
#include <Mirrors.hpp>
#include <UpdateWorker.hpp>

/**
 * @brief Destructor for UpdateWorker.
 */
UpdateWorker::~UpdateWorker() {
    this->requestInterruption();
    this->quit();
    this->wait();
}

/**
 * @brief Main execution loop for the update worker thread.
 */
void UpdateWorker::run() {
    check();

    if (this->isInterruptionRequested())
        return;

    QTimer timer;
    timer.setInterval(1000 * 3600); // 60 minutos
    connect(&timer, &QTimer::timeout, this, &UpdateWorker::check);
    timer.start();

    exec();
    timer.stop();
}

/**
 * @brief Checks whether the package database requires an update.
 */
void UpdateWorker::check() {
    if (this->isInterruptionRequested() || updatePending.load(std::memory_order_relaxed))
        return;

    emit checkStarted();

    if (Mirrors::isUpdateRequired()) {
        updatePending.store(true, std::memory_order_relaxed);
        emit updateAvailable();
    }

    emit checkFinished();
}
