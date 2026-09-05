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
#include "fcitx-utils/coroutine.h"
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
    void setPortal(const std::string &name);

    const std::string &shellName() const { return shellName_; }
    std::optional<uint32_t> shellPid() const { return shellPid_; }

    Signal<void()> shellNameChanged;

    void updateState();

private:
    void init();
    void reset();

    Coroutine<void> initShell();
    void initMonitorBus();

    dbus::Bus *bus_;

    // monitoring resources
    std::unique_ptr<dbus::ServiceWatcher> serviceWatcher_;
    std::unique_ptr<dbus::ServiceWatcherEntry> handleShell_;
    std::unique_ptr<dbus::ServiceWatcherEntry> handlePortal_;

    // Precondition to start init
    std::string shellName_;
    std::string portalName_;

    // init shell task.
    std::optional<CoroutineTask<void>> initShellTask_;

    // init shell result.
    std::unique_ptr<dbus::Bus> monitorBus_;
    std::unique_ptr<dbus::Slot> filter_;
    std::optional<uint32_t> shellPid_;
    std::unique_ptr<dbus::Slot> propertyChangedSlot_;
    bool overviewActive_ = false;

    // State.
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
