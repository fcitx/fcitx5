/*
 * SPDX-FileCopyrightText: 2026~2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_IBUSFRONTEND_GNOMEAPPMONITOR_H_
#define _FCITX5_FRONTEND_IBUSFRONTEND_GNOMEAPPMONITOR_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/matchrule.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/signals.h"
#include "appmonitor.h"

namespace fcitx {
class GnomeWindow;

class GnomeAppMonitor : public AppMonitor {
public:
    GnomeAppMonitor(dbus::Bus *bus);
    ~GnomeAppMonitor() override;

    bool isAvailable() const override;
    void setShell(const std::string &name);
    void setShellPid(uint32_t pid);
    void setPortal(const std::string &name);

    const std::string &shellName() const { return shellName_; }
    std::optional<uint32_t> shellPid() const { return shellPid_; }

    void refreshState();

    Signal<void()> shellNameChanged;

    void getOverviewActive();

    void updateState();

private:
    dbus::Bus *bus_;
    std::unique_ptr<dbus::ServiceWatcher> serviceWatcher_;
    std::unique_ptr<dbus::ServiceWatcherEntry> handleShell_;
    std::unique_ptr<dbus::ServiceWatcherEntry> handlePortal_;
    std::unique_ptr<dbus::Bus> monitorBus_;
    std::unique_ptr<dbus::Slot> filter_;
    std::unique_ptr<dbus::Slot> shellPidSlot_;
    std::string shellName_;
    std::optional<uint32_t> shellPid_;
    std::string portalName_;

    std::unique_ptr<dbus::Slot> propertyChangedSlot_;
    std::unique_ptr<dbus::Slot> getSlot_;

    bool overviewActive_ = false;
    std::unordered_set<std::string> apps_;
    std::string focus_;

    uint64_t lastSerial_ = 0;

    static constexpr char shellDBusName[] = "org.gnome.Shell";
    static constexpr char introspectName[] = "org.gnome.Shell.Introspect";
    static constexpr char gnomePortalName[] =
        "org.freedesktop.impl.portal.desktop.gnome";

    dbus::MatchRule replyRule_{dbus::MessageType::Reply,
                               introspectName,
                               gnomePortalName,
                               "",
                               "",
                               "",
                               {},
                               true};
    dbus::MatchRule getRunningAppRule_{dbus::MessageType::MethodCall,
                                       gnomePortalName,
                                       introspectName,
                                       "/org/gnome/Shell/Introspect",
                                       "org.gnome.Shell.Introspect",
                                       "GetRunningApplications"};
    dbus::MatchRule shellPropertyChangedRule_{shellDBusName, "/org/gnome/Shell",
                                              "org.freedesktop.DBus.Properties",
                                              "PropertiesChanged"};
};

} // namespace fcitx

#endif // _FCITX5_FRONTEND_WAYLANDIM_GNOMEAPPMONITOR_H_
