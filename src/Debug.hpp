/**
 * @file Debug.cpp
 * @brief Header for Debug system.
 */

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <QVariantList>
#include <Version.hpp>

#define I(x) QString::number(x)

namespace Debug {
#ifdef DEBUG
#define GREEN       "\x1b[38;2;000;255;000m"
#define RED         "\x1b[38;2;255;050;050m"
#define YELLOW      "\x1b[38;2;255;255;000m"
#define ORANGE      "\x1b[38;2;255;140;060m"
#define CYAN        "\x1b[38;2;000;255;255m"
#define DARKGREEN   "\x1b[38;2;000;150;020m"
#define BLUE        "\x1b[38;2;000;120;255m"
#define VIOLET      "\x1b[38;2;160;120;255m"
#define LIGHTRED    "\x1b[38;2;255;100;100m"
#define BLUEGREEN   "\x1b[38;2;000;150;100m"
#define YELLOWGREEN "\x1b[38;2;170;255;000m"
#define LIGHTGREEN  "\x1b[38;2;085;255;050m"
#define PURPLE      "\x1b[38;2;100;100;255m"

#define MESSAGE(x) qDebug().nospace() << GREEN << "(" << RED << x << GREEN << ")" << RED << "::"
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
        explicit Debug();

        void msg(const QString &str, const QString &name = "DEBUG", const QVariantList &args = QVariantList()) const;

    private:
        static bool isType(const QVariant &v, const QVariant &t) { return v.typeId() == t.typeId(); }

        QStringList list_color{};
    };
}

#endif
