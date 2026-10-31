/**
 * @file UpdateWorker.hpp
 * @brief Header for asynchronous background update checking thread.
 */

#ifndef UPDATEWORKER_HPP
#define UPDATEWORKER_HPP

#include <QThread>

/**
 * @class UpdateWorker
 * @brief Dedicated background thread for periodic package database update checking.
 */
class UpdateWorker final : public QThread {
    Q_OBJECT

public:
    explicit UpdateWorker(QObject *parent = nullptr);

    ~UpdateWorker() override;

signals:
    void updateAvailable();

protected:
    void run() override;

private:
    void check();
};

#endif
