/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_PORTALAPPMONITOR_H_
#define _FCITX5_FRONTEND_WAYLANDIM_PORTALAPPMONITOR_H_

#include "fcitx-utils/dbus/servicewatcher.h"

namespace fcitx {

class PortalAppMonitor {
public:
    PortalAppMonitor(dbus::Bus *bus);

    void refreshAppState();

private:
    dbus::Bus *bus_;
    dbus::ServiceWatcher watcher_;
    std::vector<std::unique_ptr<dbus::ServiceWatcherEntry>> entries_;
    std::string currentOwner_;
    std::unique_ptr<dbus::Slot> changedSignalSlot_;
    std::unique_ptr<dbus::Slot> getAppStateSlot_;
    std::unordered_map<std::string, uint32_t> appState_;
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_PORTALAPPMONITOR_H_
