/**
 * @file Process.cpp
 * @brief Custom Process class that wraps QProcess to capture and log output.
 */

#include <Debug.hpp>
#include <Process.hpp>

/**
 * @brief Constructs a Process object and connects its output streams.
 * @param parent Pointer to the parent object.
 */
Process::Process(QObject *parent) : QProcess(parent) {
    connect(this, &QProcess::readyReadStandardOutput, this, [this] { processOutput(false); });
    connect(this, &QProcess::readyReadStandardError, this, [this] { processOutput(true); });
}

/**
 * @brief Processes output from the running process.
 * @param isError Whether the data comes from the standard error stream.
 */
void Process::processOutput(const bool isError) {
    if (const QByteArray data = isError ? readAllStandardError() : readAllStandardOutput(); !data.isEmpty())
        Debug::Debug().msg(data, "Helper");
}
