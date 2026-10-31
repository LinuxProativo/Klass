/**
 * @file UpdateWorker.cpp
 * @brief Implementation of asynchronous background update checking thread.
 */

#include <QTimer>
#include <Mirrors.hpp>
#include <UpdateWorker.hpp>

/**
 * todo
 * @param parent todo
 */
UpdateWorker::UpdateWorker(QObject *parent) : QThread(parent) {}

/**
 * todo
 */
UpdateWorker::~UpdateWorker() {
    requestInterruption();
    quit();
    wait();
}

/**
 * @brief TODO
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
