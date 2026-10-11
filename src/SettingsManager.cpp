/**
 * @file SettingsManager.cpp
 * @brief Handles application settings persistence and general options.
 */

#include <Debug.hpp>
#include <SettingsManager.hpp>

/**
 * @brief Initializes the settings manager and applies stored window geometry.
 * @param window Pointer to the main window to be managed/resized.
 */
SettingsManager::SettingsManager(QWidget *window) : win(window) {
    if (win) {
        Debug::msg("Primary screen resolution", "SettingsManager",
                   {QString("%1x%2").arg(I(rect.width()), I(rect.height()))});

        win->setMinimumSize(defSize());
        win->setGeometry(windowGeometry());
    }
}

/**
 * @brief Sets the maximization state preference.
 * @param b True if the window should be maximized, false otherwise.
 */
void SettingsManager::windowMaximize(const bool b) {
    this->setValue("WindowMaximize", b);
}

/**
 * @brief Retrieves the stored maximization state.
 * @return The saved maximization state.
 */
bool SettingsManager::windowMaximize() const {
    return this->value("WindowMaximize", false).toBool();
}

/**
 * @brief Saves the current window geometry.
 * @param rec The QRect representing the window's position and size.
 */
void SettingsManager::windowGeometry(const QRect rec) {
    this->setValue("WindowGeometry", rec);
}

/**
 * @brief Retrieves the stored window geometry or provides default centered coordinates.
 * @return The saved geometry or a centered default if none exists.
 */
QRect SettingsManager::windowGeometry() const {
    return this->value("WindowGeometry", QRect(center().x(), center().y(),
                                               defSize().width(), defSize().height())).toRect();
}

/**
 * @brief Sets the application autostart preference.
 * @param b True to enable autostart on boot, false otherwise.
 */
void SettingsManager::autostart(const bool b) {
    this->setValue("Autostart", b);
}

/**
 * @brief Retrieves the stored autostart preference.
 * @return True if autostart is enabled, false by default.
 */
bool SettingsManager::autostart() const {
    return this->value("Autostart", false).toBool();
}

/**
 * @brief Sets the preference to start the application maximized.
 * @param b True to start maximized.
 */
void SettingsManager::startMaximized(const bool b) {
    this->setValue("StartMaximized", b);
}

/**
 * @brief Retrieves the preference to start the application maximized.
 * @return True if start maximized is enabled, false by default.
 */
bool SettingsManager::startMaximized() const {
    return this->value("StartMaximized", false).toBool();
}

/**
 * @brief Sets the preference to start the application minimized to the system tray.
 * @param b True to start minimized.
 */
void SettingsManager::startMinimized(const bool b) {
    this->setValue("StartMinimized", b);
}

/**
 * @brief Retrieves the preference to start the application minimized.
 * @return True if start minimized is enabled, false by default.
 */
bool SettingsManager::startMinimized() const {
    return this->value("StartMinimized", false).toBool();
}

/**
 * @brief Sets the preference to show a confirmation dialog before selecting all updates.
 * @param b True to show dialog.
 */
void SettingsManager::confirmSelectAllUpdates(const bool b) {
    this->setValue("ConfirmSelectAllUpdates", b);
}

/**
 * @brief Retrieves whether to confirm before selecting all updates.
 * @return True by default.
 */
bool SettingsManager::confirmSelectAllUpdates() const {
    return this->value("ConfirmSelectAllUpdates", true).toBool();
}

/**
 * @brief Sets whether update selection is limited to official repositories only.
 * @param b True to limit to official mirrors.
 */
void SettingsManager::officialUpdatesOnly(const bool b) {
    this->setValue("OfficialUpdatesOnly", b);
}

/**
 * @brief Retrieves whether update selection is limited to official repositories only.
 * @return False by default.
 */
bool SettingsManager::officialUpdatesOnly() const {
    return this->value("OfficialUpdatesOnly", false).toBool();
}

/**
 * @brief Sets the preference for attaching the patches repository to the official repository.
 * @param b True to attach patches repository, false otherwise.
 */
void SettingsManager::attachPatches(const bool b) {
    this->setValue("AttachPatches", b);
}

/**
 * @brief Retrieves the stored attach patches preference.
 * @return True if patches repository attachment is enabled, false by default.
 */
bool SettingsManager::attachPatches() const {
    return this->value("AttachPatches", false).toBool();
}

/**
 * @brief Sets the preference for attaching the testing repository to the official repository.
 * @param b True to attach testing repository, false otherwise.
 */
void SettingsManager::attachTesting(const bool b) {
    this->setValue("AttachTesting", b);
}

/**
 * @brief Retrieves the stored attach testing preference.
 * @return True if testing repository attachment is enabled, false by default.
 */
bool SettingsManager::attachTesting() const {
    return this->value("AttachTesting", false).toBool();
}
