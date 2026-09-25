/**
 * @file Debug.cpp
 * @brief Custom debugging utility to facilitate CLI-based troubleshooting with color-coded log outputs.
 */

#include <Debug.hpp>

#include <array>

#ifdef DEBUG
constexpr std::array<const char *, 13> COLOR_TABLE = {
    "\x1b[38;2;000;255;000m", // 0: Green
    "\x1b[38;2;255;050;050m", // 1: Red
    "\x1b[38;2;255;255;000m", // 2: Yellow
    "\x1b[38;2;255;140;060m", // 3: Orange
    "\x1b[38;2;000;255;255m", // 4: Cyan
    "\x1b[38;2;000;150;020m", // 5: DarkGreen
    "\x1b[38;2;000;120;255m", // 6: Blue
    "\x1b[38;2;160;120;255m", // 7: Violet
    "\x1b[38;2;255;100;100m", // 8: LightRed
    "\x1b[38;2;000;150;100m", // 9: BlueGreen
    "\x1b[38;2;170;255;000m", // 10: YellowGreen
    "\x1b[38;2;085;255;050m", // 11: LightGreen
    "\x1b[38;2;100;100;255m" // 12: Purple
};

static const char *getColor(const int index) {
    if (index >= 0 && static_cast<std::size_t>(index) < COLOR_TABLE.size())
        return COLOR_TABLE[static_cast<std::size_t>(index)];

    return COLOR_TABLE[DColor::Yellow];
}
#endif

/**
 * @brief Formats and outputs a log message with optional string parameters and color highlighting.
 * @param str The message to display.
 * @param name The component name to identify the log source (defaults to "DEBUG").
 * @param args A list of arguments to format (can contain string data or Color enums).
 */
void Debug::msg(const QString &str, const QString &name, const QVariantList &args) {
#ifdef DEBUG
    QString parm{}, p{}, fcolor{}, scolor{};
    const QVariant stringSentinel{QString()};
    const QVariant colorSentinel{DColor::Color()};

    for (const QVariant &arg: args) {
        if (arg.typeId() == QMetaType::QString) {
            parm = arg.toString();
            p = ": ";
        } else if (arg.typeId() == QMetaType::Int) {
            const int colorIdx = arg.toInt();
            if (fcolor.isEmpty())
                fcolor = getColor(colorIdx);
            else if (scolor.isEmpty())
                scolor = getColor(colorIdx);
        }
    }

    MESSAGE(STR(name)) << STR(fcolor.isEmpty() ? getColor(DColor::Yellow) : fcolor)
            << STR(QString("%1%2").arg(str, p))
            << STR((scolor.isEmpty() && !parm.isEmpty()) ? getColor(DColor::Yellow) : scolor)
            << STR(parm) << "\x1b[m";
#endif
}
