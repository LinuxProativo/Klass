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
    Debug::Debug().msg("Start Worker for CheckUpdate", "UpdateWorker");
    check();

    timer = new QTimer(this);
    timer->setInterval(1000 * 60 * 30);
    connect(timer, &QTimer::timeout, this, &UpdateWorker::check);
    timer->start();
}

/**
 * @brief Checks whether the package database requires an update.
 */
void UpdateWorker::check() {
    if (Mirrors::isUpdateRequired())
        emit updateAvailable();
}
