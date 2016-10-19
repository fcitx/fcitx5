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
#ifndef _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_
#define _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_

#include <fcitx/addoninstance.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>
#include "modules/wayland/wayland_public.h"
#include "wayland-input-method-unstable-v1-client-protocol.h"

namespace fcitx {

class WaylandIMModule;
class WaylandIMServer;

class WaylandIMModule : public AddonInstance {
public:
    WaylandIMModule(Instance *instance);
    ~WaylandIMModule();

    AddonInstance *wayland();
    Instance *instance() { return instance_; }

private:
    Instance *instance_;
    std::unordered_map<std::string, std::unique_ptr<WaylandIMServer>> servers_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>> createdCallback_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>> closedCallback_;
};

class WaylandIMModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override { return new WaylandIMModule(manager->instance()); }
};
}

FCITX_ADDON_FACTORY(fcitx::WaylandIMModuleFactory);

#endif // _FCITX_FRONTEND_WAYLANDIM_WAYLANDIM_H_
