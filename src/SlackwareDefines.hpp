/**
 * @file SlackwareDefines.hpp
 * @brief Global constants and system paths for the Klass Package Manager.
 */

#ifndef SLACKWARE_DEFINES_HPP
#define SLACKWARE_DEFINES_HPP

#include <QColor>
#include <QObject>
#include <QStandardPaths>

// Socket
inline const auto SOCKET_PATH = "klass_helper_socket";

//Communication
inline constexpr char SEP = '\x1E';
inline const auto TASK = "TASK:";
inline constexpr int TASK_SIZE = 5;
inline const auto OUTPUT = "OUT:";
inline constexpr int OUTPUT_SIZE = 4;
inline const auto LOG = "LOG:";
inline constexpr int LOG_SIZE = 4;
inline const auto ERROR = "ERR:";
inline constexpr int ERROR_SIZE = 4;
inline const auto INPUT_CHAR = "INPUT:";
inline constexpr int INPUT_SIZE = 6;

// Locals
inline const auto KLASS_DATABASE = QStringLiteral("/var/lib/klass/database/");
inline const auto KLASS_PACKAGES = QStringLiteral("/var/lib/klass/packages/");
inline const auto SLACK_PACKAGES = QStringLiteral("/var/lib/pkgtools/packages/");
inline const auto SLACKPKG_MIRRORS = QStringLiteral("/etc/slackpkg/mirrors");
inline const auto CONFIG_PATH = QStringLiteral("/var/lib/klass/klass.conf");
inline const auto HOME_CONFIG = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
inline const auto AUTOSTART_CONFIG = HOME_CONFIG + QStringLiteral("/autostart");
inline const auto DESKTOP_FILE = QStringLiteral("klass.desktop");
inline const auto DESKTOP_FILE_PATH = AUTOSTART_CONFIG + QStringLiteral("/") + DESKTOP_FILE;

// Repositories
inline const auto SLACK_OFICIAL = QStringLiteral("Slackware");
inline const auto SLACK_PATCHES = QStringLiteral("Slackware-Patches");
inline const auto SLACK_TESTING = QStringLiteral("Slackware-Testing");
inline const auto SLACK_EXTRA = QStringLiteral("Slackware-Extra");
inline const auto SLACK_OTHERS = QStringLiteral("Uncategorized");
inline const auto ALL_REPOSITORIES = QStringLiteral("Multiple");

//Colors
inline constexpr auto COLOR_PURPLE = QColor(134,67,194);
inline constexpr auto COLOR_BLUE = QColor(61,174,233);
inline constexpr auto COLOR_GREEN = QColor(46,204,113);
inline constexpr auto COLOR_YELLOW = QColor(253,188,75);
inline constexpr auto COLOR_ORANGE = QColor(246,116,0);
inline constexpr auto COLOR_RED = QColor(218,68,83);

#endif
