/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#ifndef _DBUS_DBUSMODULE_H_
#define _DBUS_DBUSMODULE_H_

#include "fcitx/addoninstance.h"
#include "fcitx/addonfactory.h"
#include "fcitx-utils/dbus.h"
#include "fcitx/instance.h"

namespace fcitx {
class Controller1;
class DBusModule : public AddonInstance {
public:
    DBusModule(Instance *instance);
    ~DBusModule();

private:
    std::unique_ptr<dbus::Bus> m_bus;
    std::unique_ptr<Controller1> m_controller;
};

class DBusModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override;
};

FCITX_ADDON_FACTORY(DBusModuleFactory)
}

#endif // _DBUS_DBUSMODULE_H_
