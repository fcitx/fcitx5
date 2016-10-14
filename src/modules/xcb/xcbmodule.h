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
#ifndef _FCITX_MODULES_XCB_XCBMODULE_H_
#define _FCITX_MODULES_XCB_XCBMODULE_H_

#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>

// workaround xkb.h using explicit keyword problem
#define explicit no_cxx_explicit
#include <xcb/xkb.h>
#undef explicit

#include "fcitx-utils/event.h"
#include "fcitx-utils/handlertable.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "xcb_public.h"
#include <list>
#include <unordered_map>
#include <vector>

namespace fcitx {

class XCBModule;

class XCBConnection {
public:
    XCBConnection(XCBModule *xcb, const std::string &name);
    ~XCBConnection();

    void updateKeymap();
    HandlerTableEntry<XCBEventFilter> *addEventFilter(XCBEventFilter filter);

    const std::string &name() const { return name_; }
    xcb_connection_t *connection() const { return conn_.get(); }
    int screen() const { return screen_; }
    FocusGroup *focusGroup() const { return group_; }
    struct xkb_state *xkbState() {
        return state_.get();
    }
    XkbRulesNames xkbRulesNames();

private:
    bool filterEvent(xcb_connection_t *conn, xcb_generic_event_t *event);
    void onIOEvent();

    XCBModule *parent_;
    std::string name_;
    std::unique_ptr<xcb_connection_t, decltype(&xcb_disconnect)> conn_;
    int screen_;
    xcb_atom_t atom_;
    xcb_window_t serverWindow_;
    xcb_window_t root_;
    FocusGroup *group_;

    bool hasXKB_;
    xcb_atom_t xkbRulesNamesAtom_;
    uint8_t xkbFirstEvent_;
    int32_t coreDeviceId_;

    std::unique_ptr<struct xkb_context, decltype(&xkb_context_unref)> context_;
    std::unique_ptr<struct xkb_keymap, decltype(&xkb_keymap_unref)> keymap_;
    std::unique_ptr<struct xkb_state, decltype(&xkb_state_unref)> state_;

    std::unique_ptr<EventSourceIO> ioEvent_;

    HandlerTable<XCBEventFilter> filters_;
    // need to be clean up before filters_ destructs;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> filter_;
};

class XCBModule : public AddonInstance {
public:
    XCBModule(Instance *instance);

    void openConnection(const std::string &name);
    void removeConnection(const std::string &name);
    Instance *instance() { return instance_; }

    HandlerTableEntry<XCBEventFilter> *addEventFilter(const std::string &name, XCBEventFilter filter);
    HandlerTableEntry<XCBConnectionCreated> *addConnectionCreatedCallback(XCBConnectionCreated callback);
    HandlerTableEntry<XCBConnectionClosed> *addConnectionClosedCallback(XCBConnectionClosed callback);
    struct xkb_state *xkbState(const std::string &name);
    XkbRulesNames xkbRulesNames(const std::string &name);

private:
    void onConnectionCreated(XCBConnection &conn);
    void onConnectionClosed(XCBConnection &conn);

    Instance *instance_;
    std::unordered_map<std::string, XCBConnection> conns_;
    HandlerTable<XCBConnectionCreated> createdCallbacks_;
    HandlerTable<XCBConnectionClosed> closedCallbacks_;
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addEventFilter);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addConnectionCreatedCallback);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addConnectionClosedCallback);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, xkbState);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, xkbRulesNames);
};

class XCBModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override { return new XCBModule(manager->instance()); }
};
}

FCITX_ADDON_FACTORY(fcitx::XCBModuleFactory);

#endif // _FCITX_MODULES_XCB_XCBMODULE_H_
