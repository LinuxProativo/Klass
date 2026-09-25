/**
 * @file UpdateWorker.cpp
 * @brief Periodically checks whether the package database is outdated.
 */

#include <Debug.hpp>
#include <Mirrors.hpp>
#include <UpdateWorker.hpp>

/**
 * @brief Starts periodic update checks.
 */
void UpdateWorker::start() {
    Debug::msg("Start Worker for CheckUpdate", "UpdateWorker");
    check();

    QTimer updateTimer;
    updateTimer.setInterval(1000 * 60 * 30);
    connect(&updateTimer, &QTimer::timeout, this, &UpdateWorker::check);
    updateTimer.start();
}

/**
 * @brief Checks whether the package database requires an update.
 */
void UpdateWorker::check() {
    if (Mirrors::isUpdateRequired())
        emit updateAvailable();
}
