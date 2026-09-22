/**
 * @file SettingsManager.hpp
 * @brief Header for config using QSettings.
 */

#ifndef SETTINGSMANAGER_HPP
#define SETTINGSMANAGER_HPP

#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <QWidget>

/**
 * @class SettingsManager
 * @brief Manages the saving and loading of user preferences and window state using QSettings.
 */
class SettingsManager : public QSettings {
    Q_OBJECT

public:
    explicit SettingsManager(QWidget *window = nullptr);

    static QSize defSize() { return {960, 540}; }

    static QSize res() { return QGuiApplication::primaryScreen()->geometry().size(); }

    void windowMaximize(bool b);

    [[nodiscard]] bool windowMaximize() const;

    void windowGeometry(QRect rec);

    [[nodiscard]] QRect windowGeometry() const;

    void autostart(bool b);

    [[nodiscard]] bool autostart() const;

    void attachPatches(bool b);

    [[nodiscard]] bool attachPatches() const;

    void attachTesting(bool b);

    [[nodiscard]] bool attachTesting() const;

private:
    [[nodiscard]] QPoint center() const { return rect.center() - win->frameGeometry().center(); }

    QWidget *win{};
    QRect rect{QGuiApplication::primaryScreen()->geometry()};
};

#endif
