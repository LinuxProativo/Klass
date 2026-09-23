/**
 * @file Debug.hpp
 * @brief Header for Debug system.
 */

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <QVariantList>
#include <QDebug>

#include <Version.hpp>

#define I(x) QString::number(x)

namespace Debug {
#ifdef DEBUG
    constexpr auto ANSI_GREEN = "\x1b[38;2;000;255;000m";
    constexpr auto ANSI_RED = "\x1b[38;2;255;050;050m";

#define MESSAGE(x) qDebug().nospace() << ANSI_GREEN << "(" << ANSI_RED << x << ANSI_GREEN << ")" << ANSI_RED << "::"
#define STR(x) qUtf8Printable(x)
#endif

    enum Color {
        Green = 0,
        Red = 1,
        Yellow = 2,
        Orange = 3,
        Cyan = 4,
        DarkGreen = 5,
        Blue = 6,
        Violet = 7,
        LightRed = 8,
        BlueGreen = 9,
        YellowGreen = 10,
        LightGreen = 11,
        Purple = 12
    };

    /**
     * @class Debug
     * @brief Manages terminal color configurations and formatting for debug messages.
     */
    class Debug final : public QObject {
        Q_OBJECT

    public:
        explicit Debug() = default;

        static void msg(const QString &str, const QString &name = "DEBUG", const QVariantList &args = {});
    };
}

#endif
