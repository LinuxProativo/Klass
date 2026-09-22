/**
 * @file StatusIcons.hpp
 * @brief Header file defining package status states and a high-performance, pre-allocated icon manager.
 */

#ifndef STATUSICONS_HPP
#define STATUSICONS_HPP

#include <QIcon>
#include <QtGlobal>

#include <array>

/**
 * @enum PkgStatus
 * @brief Identifies the state or queued operation of a package.
 */
enum class PkgStatus {
    Reset = 0,
    Available,
    Installed,
    Excluded,
    PendingInstall,
    PendingUpdate,
    PendingReinstall,
    PendingRemove,
    Count
};

/**
 * @class StatusIcons
 * @brief High-performance provider for pre-allocated package status icons.
 */
class StatusIcons {
public:
    static void init();

    static const QIcon &statusIcon(PkgStatus status);

private:
    static bool initialized;
    static std::array<QIcon, static_cast<size_t>(PkgStatus::Count)> icons;
};

#endif
