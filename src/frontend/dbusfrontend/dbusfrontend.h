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
#ifndef _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_
#define _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_

#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/event.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"

namespace fcitx {

class AddonInstance;
class Instance;
class InputMethod1;

class DBusFrontendModule : public AddonInstance {
public:
    DBusFrontendModule(Instance *instance);
    ~DBusFrontendModule();

    AddonInstance *dbus();
    dbus::Bus *bus();
    Instance *instance() { return instance_; }

    dbus::ServiceWatcher &serviceWatcher() { return *watcher_; }

private:
    Instance *instance_;
    std::unique_ptr<InputMethod1> inputMethod1_;
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
};

class DBusFrontendModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override { return new DBusFrontendModule(manager->instance()); }
};
}

FCITX_ADDON_FACTORY(fcitx::DBusFrontendModuleFactory);

#endif // _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_
