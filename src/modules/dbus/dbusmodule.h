/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_DBUS_DBUSMODULE_H_
#define _FCITX_MODULES_DBUS_DBUSMODULE_H_

#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "dbus_public.h"

namespace fcitx {
class Controller1;
class DBusModule : public AddonInstance {
public:
    DBusModule(Instance *instance);
    ~DBusModule();

    dbus::Bus *bus();
    bool lockGroup(int group);
    FCITX_ADDON_DEPENDENCY_LOADER(keyboard, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(xcb, instance_->addonManager());

private:
    FCITX_ADDON_EXPORT_FUNCTION(DBusModule, bus);
    FCITX_ADDON_EXPORT_FUNCTION(DBusModule, lockGroup);

    std::unique_ptr<dbus::Bus> connectToSessionBus();

    Instance *instance_;
    std::unique_ptr<dbus::Bus> bus_;
    std::unique_ptr<dbus::Slot> disconnectedSlot_;
    std::unique_ptr<dbus::ServiceWatcher> serviceWatcher_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>>
        selfWatcher_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>>
        xkbWatcher_;
    std::string xkbHelperName_;
    std::unique_ptr<Controller1> controller_;
};
} // namespace fcitx

#endif // _FCITX_MODULES_DBUS_DBUSMODULE_H_
