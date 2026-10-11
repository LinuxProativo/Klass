/**
 * @file DefaultPath.hpp
 * @brief Header for manage file and directory path.
 */

#ifndef DEFAULTPATH_HPP
#define DEFAULTPATH_HPP

#include <QString>

/**
 * @class DefaultPath
 * @brief Manages file and directory path discovery, supporting fallbacks between local dirs and system paths.
 */
class DefaultPath final {
public:
    explicit DefaultPath() = default;

    static QString defaultPath(const QString &str);

private:
    static QString findPath(const QString &str);
};

#endif
