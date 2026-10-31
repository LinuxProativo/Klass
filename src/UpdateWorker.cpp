/**
 * @file UpdateWorker.cpp
 * @brief Implementation of the asynchronous background update checking thread.
 */

#include <QTimer>
#include <Mirrors.hpp>
#include <UpdateWorker.hpp>

/**
 * @brief Constructor for UpdateWorker.
 * @param parent The parent QObject for memory management.
 */
UpdateWorker::UpdateWorker(QObject *parent) : QThread(parent) {}

/**
 * @brief Destructor for UpdateWorker.
 */
UpdateWorker::~UpdateWorker() {
    requestInterruption();
    quit();
    wait();
}

/**
 * @brief Main execution loop for the update worker thread.
 */
void UpdateWorker::run() {
    check();

    if (isInterruptionRequested())
        return;

    QTimer timer;
    timer.setInterval(1000 * 60 * 30); // 30 minutos
    connect(&timer, &QTimer::timeout, this, &UpdateWorker::check);
    timer.start();

    exec();
    timer.stop();
}

/**
 * @brief Checks whether the package database requires an update.
 */
void UpdateWorker::check() {
    if (isInterruptionRequested())
        return;

    if (Mirrors::isUpdateRequired())
        emit updateAvailable();
}
