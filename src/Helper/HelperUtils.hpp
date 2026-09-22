/**
 * @file HelperUtils.hpp
 * @brief Header for root operations.
 */

#ifndef HELPERUTILS_HPP
#define HELPERUTILS_HPP

#include <QList>
#include <QLocalSocket>

#include <TaskManager.hpp>

/**
 * @struct TransactionPkg
 * @brief Represents a package prepared for download and installation inside the Helper context.
 */
struct TransactionPkg {
    QString action{};
    QString name{};
    QString version{};
    QString repo{};
    QString category{};
    QString url{};
    QString ascUrl{};
    QString md5{};
    QString ascMd5{};
};

/**
 * @class HelperUtils
 * @brief Provides utility methods and background tasks for package management operations.
 */
class HelperUtils : QObject {
    Q_OBJECT

public:
    explicit HelperUtils() = default;

    void performUpdate(QLocalSocket *socket);

    void processTransaction(QLocalSocket *socket, TaskManager *task, const QList<TransactionPkg> &pkgs);

    void setOperationCancelled(const bool u) { operationCancelled = u; }

    static bool checkSocketCancellation(QLocalSocket *socket, bool &cancelledFlag);

private:
    static QString calculateLocalMd5(const QString &filePath);

    bool operationCancelled{false};
};

#endif
