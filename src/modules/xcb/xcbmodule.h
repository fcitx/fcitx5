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

typedef std::function<void(xcb_atom_t selection)> XCBSelectionNotifyCallback;

class XCBModule;

class XCBConnection {
public:
    XCBConnection(XCBModule *xcb, const std::string &name);
    ~XCBConnection();

    void updateKeymap();
    HandlerTableEntry<XCBEventFilter> *addEventFilter(XCBEventFilter filter);
    HandlerTableEntry<XCBSelectionNotifyCallback> *addSelection(const std::string &name,
                                                                XCBSelectionNotifyCallback callback);

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
    void addSelectionAtom(xcb_atom_t atom);
    void removeSelectionAtom(xcb_atom_t atom);
    void initAtom();
    xcb_atom_t atom(const std::string &atomName, bool exists);
    void refreshCompositeManager();

    std::unordered_map<std::string, xcb_atom_t> atomCache_;

    XCBModule *parent_;
    std::string name_;
    std::unique_ptr<xcb_connection_t, decltype(&xcb_disconnect)> conn_;
    int screen_;
    xcb_atom_t atom_;
    xcb_window_t serverWindow_;
    xcb_window_t root_;
    FocusGroup *group_;

    xcb_atom_t typeMenuAtom_ = XCB_ATOM_NONE;
    xcb_atom_t windowTypeAtom_ = XCB_ATOM_NONE;
    xcb_atom_t typeDialogAtom_ = XCB_ATOM_NONE;
    xcb_atom_t typeDockAtom_ = XCB_ATOM_NONE;
    xcb_atom_t typePopupMenuAtom_ = XCB_ATOM_NONE;
    xcb_atom_t pidAtom_ = XCB_ATOM_NONE;
    xcb_atom_t utf8Atom_ = XCB_ATOM_NONE;
    xcb_atom_t stringAtom_ = XCB_ATOM_NONE;
    xcb_atom_t compTextAtom_ = XCB_ATOM_NONE;
    xcb_atom_t compMgrAtom_ = XCB_ATOM_NONE;

    std::string compMgrAtomString_;
    xcb_window_t compMgrWindow_ = XCB_WINDOW_NONE;

    bool hasXKB_;
    xcb_atom_t xkbRulesNamesAtom_;
    uint8_t xkbFirstEvent_;
    int32_t coreDeviceId_;

    bool hasXFixes_ = false;
    MultiHandlerTable<xcb_atom_t, XCBSelectionNotifyCallback> selections_{
        [this](xcb_atom_t selection) { addSelectionAtom(selection); },
        [this](xcb_atom_t selection) { removeSelectionAtom(selection); }};

    std::unique_ptr<struct xkb_context, decltype(&xkb_context_unref)> context_;
    std::unique_ptr<struct xkb_keymap, decltype(&xkb_keymap_unref)> keymap_;
    std::unique_ptr<struct xkb_state, decltype(&xkb_state_unref)> state_;

    std::unique_ptr<EventSourceIO> ioEvent_;

    HandlerTable<XCBEventFilter> filters_;
    // need to be clean up before filters_ destructs;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> filter_;
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>> compositeCallback_;
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
}

#endif // _FCITX_MODULES_XCB_XCBMODULE_H_
