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

#include "display.h"
#include "fcitx-utils/event.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "wayland_public.h"

namespace fcitx {

class WaylandModule;

class WaylandConnection {
public:
    WaylandConnection(WaylandModule *wayland, const char *name);
    ~WaylandConnection();

    const std::string &name() const { return name_; }
    wayland::Display *display() const { return display_.get(); }
    FocusGroup *focusGroup() const { return group_; }

private:
    void onIOEvent(IOEventFlags flags);
    void finish();

    WaylandModule *parent_;
    std::string name_;
    // order matters, callback in ioEvent_ uses display_.
    std::unique_ptr<EventSourceIO> ioEvent_;
    std::unique_ptr<wayland::Display> display_;
    FocusGroup *group_ = nullptr;
    int error_ = 0;
};

class WaylandModule : public AddonInstance {
public:
    WaylandModule(Instance *instance);
    Instance *instance() { return instance_; }

    void openDisplay(const std::string &display);
    void removeDisplay(const std::string &name);

    HandlerTableEntry<WaylandConnectionCreated> *
    addConnectionCreatedCallback(WaylandConnectionCreated callback);
    HandlerTableEntry<WaylandConnectionClosed> *
    addConnectionClosedCallback(WaylandConnectionClosed callback);
    wl_registry *getRegistry(const std::string &name);

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
}

#endif // _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_
