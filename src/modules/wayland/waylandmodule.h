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
#ifndef _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_
#define _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_

#include <fcitx/addoninstance.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>
#include <fcitx/focusgroup.h>
#include <wayland-client.h>
#include "wayland_public.h"
#include <fcitx-utils/event.h>

namespace fcitx {
    
class WaylandModule;

class WaylandConnection {
public:
    WaylandConnection(WaylandModule *wayland, const char *name);
    ~WaylandConnection();

    const std::string &name() const { return name_; }
    wl_display *display() const { return display_.get(); }
    FocusGroup *focusGroup() const { return group_; }

private:
    void onIOEvent();
    
    WaylandModule *parent_;
    std::string name_;
    std::unique_ptr<wl_display, decltype(&wl_display_disconnect)> display_;
    std::unique_ptr<EventSourceIO> ioEvent_;
    FocusGroup *group_ = nullptr;
    int error_ = 0;
};

class WaylandModule : public AddonInstance {
public:
    WaylandModule(Instance *instance);
    Instance *instance() { return instance_; }
    
    void openDisplay(const std::string &display);
    void removeDisplay(const std::string &name);
    
    HandlerTableEntry<WaylandConnectionCreated> *addConnectionCreatedCallback(WaylandConnectionCreated callback);
    HandlerTableEntry<WaylandConnectionClosed> *addConnectionClosedCallback(WaylandConnectionClosed callback);
private:
    void onConnectionCreated(WaylandConnection &conn);
    void onConnectionClosed(WaylandConnection &conn);
    
    
    Instance *instance_;
    std::unordered_map<std::string, WaylandConnection> conns_;
    HandlerTable<WaylandConnectionCreated> createdCallbacks_;
    HandlerTable<WaylandConnectionClosed> closedCallbacks_;
    FCITX_ADDON_EXPORT_FUNCTION(WaylandModule, addConnectionCreatedCallback);
    FCITX_ADDON_EXPORT_FUNCTION(WaylandModule, addConnectionClosedCallback);
};

class WaylandModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override { return new WaylandModule(manager->instance()); }
};
}

FCITX_ADDON_FACTORY(fcitx::WaylandModuleFactory);

#endif // _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_
