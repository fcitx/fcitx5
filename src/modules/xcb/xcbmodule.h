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
#include "fcitx/addoninstance.h"
#include "fcitx/addonfactory.h"
#include "fcitx/focusgroup.h"
#include "fcitx/addonmanager.h"
#include "fcitx-utils/handlertable.h"
#include "xcb_public.h"
#include <unordered_map>
#include <list>
#include <vector>

namespace fcitx {

class XCBModule;

class XCBConnection {
public:
    XCBConnection(XCBModule *xcb, const std::string &name);
    ~XCBConnection();

    void updateKeymap();
    HandlerTableEntry<XCBEventFilter> *addEventFilter(XCBEventFilter filter);

    const std::string &name() const { return m_name; }
    xcb_connection_t *connection() const { return m_conn.get(); }
    int screen() const { return m_screen; }
    FocusGroup *focusGroup() const { return m_group; }
    struct xkb_state *xkbState() {
        return m_state.get();
    }
    XkbRulesNames xkbRulesNames();

private:
    bool filterEvent(xcb_connection_t *conn, xcb_generic_event_t *event);
    void onIOEvent();

    XCBModule *m_parent;
    std::string m_name;
    std::unique_ptr<xcb_connection_t, decltype(&xcb_disconnect)> m_conn;
    int m_screen;
    xcb_atom_t m_atom;
    xcb_window_t m_serverWindow;
    xcb_window_t m_root;
    FocusGroup *m_group;

    bool m_hasXKB;
    xcb_atom_t m_xkbRulesNamesAtom;
    uint8_t m_xkbFirstEvent;
    int32_t m_coreDeviceId;

    std::unique_ptr<struct xkb_context, decltype(&xkb_context_unref)> m_context;
    std::unique_ptr<struct xkb_keymap, decltype(&xkb_keymap_unref)> m_keymap;
    std::unique_ptr<struct xkb_state, decltype(&xkb_state_unref)> m_state;

    std::unique_ptr<EventSourceIO> m_ioEvent;

    HandlerTable<XCBEventFilter> m_filters;
    // need to be clean up before m_filters destructs;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> m_filter;
};

class XCBModule : public AddonInstance {
public:
    XCBModule(Instance *instance);

    void openConnection(const std::string &name);
    void removeConnection(const std::string &name);
    Instance *instance() { return m_instance; }

    HandlerTableEntry<XCBEventFilter> *addEventFilter(const std::string &name, XCBEventFilter filter);
    HandlerTableEntry<XCBConnectionCreated> *addConnectionCreatedCallback(XCBConnectionCreated callback);
    HandlerTableEntry<XCBConnectionClosed> *addConnectionClosedCallback(XCBConnectionClosed callback);
    struct xkb_state *xkbState(const std::string &name);
    XkbRulesNames xkbRulesNames(const std::string &name);

private:
    void onConnectionCreated(XCBConnection &conn);
    void onConnectionClosed(XCBConnection &conn);

    Instance *m_instance;
    std::unordered_map<std::string, XCBConnection> m_conns;
    HandlerTable<XCBConnectionCreated> m_createdCallbacks;
    HandlerTable<XCBConnectionClosed> m_closedCallbacks;
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
