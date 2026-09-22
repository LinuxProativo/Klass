/**
 * @file Process.hpp
 * @brief Header for Process class.
 */

#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <QObject>
#include <QProcess>

/**
 * @class Process
 * @brief Wrapper around QProcess providing application-specific process management utilities.
 */
class Process final : public QProcess {
    Q_OBJECT

public:
    explicit Process(QObject *parent = nullptr);

private:
    void processOutput(bool isError);
};

#endif
