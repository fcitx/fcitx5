/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_APPMONITOR_H_
#define _FCITX5_FRONTEND_WAYLANDIM_APPMONITOR_H_

#include "fcitx-utils/dbus/servicewatcher.h"

namespace fcitx {

class GnomeAppMonitor {
public:
    GnomeAppMonitor(dbus::Bus *bus);

private:
    void refreshAppState(dbus::Message &msg);
    void recreateMonitorBus();
    dbus::Bus *bus_;
    dbus::ServiceWatcher watcher_;
    std::vector<std::unique_ptr<dbus::ServiceWatcherEntry>> entries_;
    std::string gnomeShellOwner_;
    std::string gtkPortalOwner_;
    std::string gnomePortalOwner_;
    std::unique_ptr<dbus::Bus> monitorBus_;
    std::unique_ptr<dbus::Slot> reply_;
    std::unique_ptr<dbus::Slot> filter_;
    std::unordered_map<std::string, uint32_t> appState_;
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_APPMONITOR_H_
