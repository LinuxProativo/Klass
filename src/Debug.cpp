/**
 * @file Debug.cpp
 * @brief Custom debugging utility to facilitate CLI-based troubleshooting with color-coded log outputs.
 */

#include <Debug.hpp>

namespace Debug {
    /**
     * @brief Initializes the Debug utility and maps available terminal colors.
     */
    Debug::Debug() {
#ifdef DEBUG
        (void) Green;
        (void) Red;
        (void) Yellow;
        (void) Orange;
        (void) Cyan;
        (void) DarkGreen;
        (void) Blue;
        (void) Violet;
        (void) LightRed;
        (void) BlueGreen;
        (void) YellowGreen;
        (void) LightGreen;
        (void) Purple;

        list_color = QStringList({
            GREEN, // 0
            RED, // 1
            YELLOW, // 2
            ORANGE, // 3
            CYAN, // 4
            DARKGREEN, // 5
            BLUE, // 6
            VIOLET, // 7
            LIGHTRED, // 8
            BLUEGREEN, // 9
            YELLOWGREEN, // 10
            LIGHTGREEN, // 11
            PURPLE // 12
        });
#endif
    }

    /**
     * @brief Formats and outputs a log message with optional string parameters and color highlighting.
     * @param str The message to display.
     * @param name The component name to identify the log source (defaults to "DEBUG").
     * @param args A list of arguments to format (can contain string data or Color enums).
     */
    void Debug::msg(const QString &str, const QString &name, const QVariantList &args) const {
#ifdef DEBUG
        QString parm{}, p{}, fcolor{}, scolor{};
        const QVariant stringSentinel{QString()};
        const QVariant colorSentinel{Color()};

        for (const QVariant &arg: args) {
            if (isType(arg, stringSentinel)) {
                parm = arg.toString();
                p = ": ";
            } else if (isType(arg, colorSentinel)) {
                if (fcolor.isEmpty())
                    fcolor = list_color.at(arg.toInt());
                else if (scolor.isEmpty())
                    scolor = list_color.at(arg.toInt());
            }
        }

        MESSAGE(STR(name)) << STR(fcolor.isEmpty() ? list_color.at(Color::Yellow) : fcolor)
                << STR(QString("%1%2").arg(str, p))
                << STR((scolor.isEmpty() && !parm.isEmpty()) ? list_color.at(Color::Yellow) : scolor)
                << STR(parm) << "\x1b[m";
#endif
    }
}
