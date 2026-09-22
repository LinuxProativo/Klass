/**
 * @file DefaultPath.hpp
 * @brief Header for manage file and directory path.
 */

#ifndef DEFAULTPATH_HPP
#define DEFAULTPATH_HPP

#include <QCoreApplication>
#include <QObject>

#include <Debug.hpp>

/**
 * @class DefaultPath
 * @brief Manages file and directory path discovery, supporting fallbacks between local dirs and system paths.
 */
class DefaultPath final : public QObject {
    Q_OBJECT

public:
    explicit DefaultPath();

    QString defaultPath(const QString &str);

private:
    QString findPath(const QString &str);

    Debug::Debug *debug{};
    QString dir{QCoreApplication::applicationDirPath()}, oldDir{};
    QString px{"/usr/local/share/klass"}, newPx{"/usr/share/klass"};
};

#endif
