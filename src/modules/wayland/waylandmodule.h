/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_
#define _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_

#include "fcitx-utils/event.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "display.h"
#include "wayland_public.h"

namespace fcitx {

class WaylandModule;

class WaylandConnection {
public:
    WaylandConnection(WaylandModule *wayland, const char *name);
    ~WaylandConnection();

    const std::string &name() const { return name_; }
    wayland::Display *display() const { return display_.get(); }
    FocusGroup *focusGroup() const { return group_.get(); }

private:
    void onIOEvent(IOEventFlags flags);
    void finish();

    WaylandModule *parent_;
    std::string name_;
    // order matters, callback in ioEvent_ uses display_.
    std::unique_ptr<EventSourceIO> ioEvent_;
    std::unique_ptr<wayland::Display> display_;
    std::unique_ptr<FocusGroup> group_;
    int error_ = 0;
};

class WaylandModule : public AddonInstance {
public:
    WaylandModule(Instance *instance);
    Instance *instance() { return instance_; }

    void openDisplay(const std::string &name);
    void removeDisplay(const std::string &name);

    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
    addConnectionCreatedCallback(WaylandConnectionCreated callback);
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>>
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
} // namespace fcitx

#endif // _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_
