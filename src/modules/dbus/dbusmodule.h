/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_MODULES_DBUS_DBUSMODULE_H_
#define _FCITX_MODULES_DBUS_DBUSMODULE_H_

#include "dbus_public.h"
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"

namespace fcitx {
class Controller1;
class DBusModule : public AddonInstance {
public:
    DBusModule(Instance *instance);
    ~DBusModule();

    dbus::Bus *bus();
    bool lockGroup(int group);
    FCITX_ADDON_DEPENDENCY_LOADER(keyboard, instance_->addonManager());

private:
    FCITX_ADDON_EXPORT_FUNCTION(DBusModule, bus);
    FCITX_ADDON_EXPORT_FUNCTION(DBusModule, lockGroup);

    std::unique_ptr<dbus::Bus> bus_;
    std::unique_ptr<dbus::ServiceWatcher> serviceWatcher_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>>
        selfWatcher_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>>
        xkbWatcher_;
    std::string xkbHelperName_;
    std::unique_ptr<Controller1> controller_;
    Instance *instance_;
};
}

#endif // _FCITX_MODULES_DBUS_DBUSMODULE_H_
