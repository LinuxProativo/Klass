/**
 * @file Fonts.cpp
 * @brief Implementation for retrieving the system font accurately while bypassing standard QFont defaults.
 */

#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

#include <array>

#include <Debug.hpp>
#include <Fonts.hpp>

/**
 * @struct FontConfigData
 * @brief Holds target paths and regular expressions required to parse font configs.
 */
struct FontConfigData {
    QStringView filePath;
    QRegularExpression regex;
};

/**
 * @brief Searches predefined configuration files (Trolltech/GTK) to extract the system font and size.
 * @return The parsed QFont object.
 */
QFont Fonts::getSystemFont() {
    QString fontName;
    int pointSize = 0;

    const std::array<FontConfigData, 2> configs = {
        {
            {u"Trolltech.conf", QRegularExpression(QStringLiteral("font=\"(.*)\""))},
            {u"gtk-3.0/settings.ini", QRegularExpression(QStringLiteral("gtk-font-name=(.*)"))}
        }
    };

    for (const auto &config: configs) {
        if (!fontName.isEmpty()) break;

        const QString fullPath = QStandardPaths::locate(QStandardPaths::ConfigLocation, config.filePath.toString());
        QFile file(fullPath);

        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                const QString line = in.readLine();
                if (const auto match = config.regex.match(line); match.hasMatch()) {
                    if (const QStringList fontInfo = match.captured(1).split(u','); fontInfo.size() >= 2) {
                        fontName = fontInfo.at(0).trimmed();
                        pointSize = fontInfo.at(1).toInt();
                        Debug::Debug().msg("System font", "Fonts",
                                           {QStringLiteral("%1, %2").arg(fontName, QString::number(pointSize))});
                        break;
                    }
                }
            }
        }
    }

    if (fontName.isEmpty())
        return QFont();

    return QFont(fontName, pointSize);
}
