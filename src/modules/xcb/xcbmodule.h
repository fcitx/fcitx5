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
#include "fcitx/instance.h"
#include "xcb_public.h"
#include <list>
#include <unordered_map>
#include <vector>
#include <xcb/xcb_keysyms.h>

namespace fcitx {
template <class T>
inline void hash_combine(std::size_t &seed, T const &v) {
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

class XCBConnection;

class ConvertSelectionRequest {
public:
    ConvertSelectionRequest() = default;
    ConvertSelectionRequest(XCBConnection *conn, xcb_atom_t selection,
                            xcb_atom_t type, xcb_atom_t property,
                            XCBConvertSelectionCallback callback);

    ConvertSelectionRequest(const ConvertSelectionRequest &) = delete;

    void handleReply(xcb_atom_t type, const char *data, size_t length);

    xcb_atom_t property() const { return property_; }
    xcb_atom_t selection() const { return selection_; }

private:
    void invokeCallbackAndCleanUp(xcb_atom_t type, const char *data,
                                  size_t length);
    void cleanUp();

    XCBConnection *conn_ = nullptr;
    xcb_atom_t selection_ = 0;
    xcb_atom_t property_ = 0;
    std::vector<xcb_atom_t> fallbacks_;
    XCBConvertSelectionCallback realCallback_;
    std::unique_ptr<EventSourceTime> timer_;
};

class XCBModule;

class XCBConnection {
public:
    XCBConnection(XCBModule *xcb, const std::string &name);
    ~XCBConnection();

    void updateKeymap();
    HandlerTableEntry<XCBEventFilter> *addEventFilter(XCBEventFilter filter);
    HandlerTableEntry<XCBSelectionNotifyCallback> *
    addSelection(const std::string &name, XCBSelectionNotifyCallback callback);

    HandlerTableEntryBase *convertSelection(const std::string &selection,
                                            const std::string &type,
                                            XCBConvertSelectionCallback);

    XCBModule *parent() { return parent_; }
    const std::string &name() const { return name_; }
    xcb_connection_t *connection() const { return conn_.get(); }
    int screen() const { return screen_; }
    FocusGroup *focusGroup() const { return group_; }
    struct xkb_state *xkbState() {
        return state_.get();
    }
    XkbRulesNames xkbRulesNames();
    xcb_window_t serverWindow() const { return serverWindow_; }

    void convertSelectionRequest(const ConvertSelectionRequest &request);
    xcb_atom_t atom(const std::string &atomName, bool exists);
    xcb_ewmh_connection_t *ewmh();

private:
    bool filterEvent(xcb_connection_t *conn, xcb_generic_event_t *event);
    void onIOEvent();
    void addSelectionAtom(xcb_atom_t atom);
    void removeSelectionAtom(xcb_atom_t atom);

    // Group enumerate.
    void setDoGrab(bool doGrab);
    void grabKey();
    void grabKey(const Key &key);
    void ungrabKey();
    void ungrabKey(const Key &key);
    bool grabXKeyboard();
    void ungrabXKeyboard();
    void keyRelease(const xcb_key_release_event_t *event);
    void acceptGroupChange();
    void navigateGroup(bool forward);

    std::unordered_map<std::string, xcb_atom_t> atomCache_;

    XCBModule *parent_;
    std::string name_;
    std::unique_ptr<xcb_connection_t, decltype(&xcb_disconnect)> conn_;
    int screen_ = 0;
    xcb_atom_t atom_ = XCB_ATOM_NONE;
    xcb_window_t serverWindow_ = XCB_WINDOW_NONE;
    xcb_window_t root_ = XCB_WINDOW_NONE;
    FocusGroup *group_ = nullptr;

    bool hasXKB_ = false;
    xcb_atom_t xkbRulesNamesAtom_ = XCB_ATOM_NONE;
    uint8_t xkbFirstEvent_ = 0;
    int32_t coreDeviceId_ = 0;

    bool hasXFixes_ = false;
    uint8_t xfixesFirstEvent_ = 0;
    MultiHandlerTable<xcb_atom_t, XCBSelectionNotifyCallback> selections_{
        [this](xcb_atom_t selection) { addSelectionAtom(selection); },
        [this](xcb_atom_t selection) { removeSelectionAtom(selection); }};

    HandlerTable<ConvertSelectionRequest> convertSelections_;

    std::unique_ptr<struct xkb_context, decltype(&xkb_context_unref)> context_;
    std::unique_ptr<struct xkb_keymap, decltype(&xkb_keymap_unref)> keymap_;
    std::unique_ptr<struct xkb_state, decltype(&xkb_state_unref)> state_;

    std::unique_ptr<EventSourceIO> ioEvent_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;

    HandlerTable<XCBEventFilter> filters_;
    // need to be clean up before filters_ destructs;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> filter_;
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
        compositeCallback_;

    xcb_ewmh_connection_t ewmh_;
    std::unique_ptr<xcb_key_symbols_t, decltype(&xcb_key_symbols_free)> syms_;

    size_t groupIndex_ = 0;
    KeyList forwardGroup_, backwardGroup_;
    bool doGrab_ = false;
    bool keyboardGrabbed_ = false;
};

class XCBModule : public AddonInstance {
public:
    XCBModule(Instance *instance);

    void openConnection(const std::string &name);
    void removeConnection(const std::string &name);
    Instance *instance() { return instance_; }

    HandlerTableEntry<XCBEventFilter> *addEventFilter(const std::string &name,
                                                      XCBEventFilter filter);
    HandlerTableEntry<XCBConnectionCreated> *
    addConnectionCreatedCallback(XCBConnectionCreated callback);
    HandlerTableEntry<XCBConnectionClosed> *
    addConnectionClosedCallback(XCBConnectionClosed callback);
    struct xkb_state *xkbState(const std::string &name);
    XkbRulesNames xkbRulesNames(const std::string &name);
    HandlerTableEntry<XCBSelectionNotifyCallback> *
    addSelection(const std::string &name, const std::string &atom,
                 XCBSelectionNotifyCallback callback);
    HandlerTableEntryBase *
    convertSelection(const std::string &name, const std::string &atom,
                     const std::string &type,
                     XCBConvertSelectionCallback callback);

    xcb_atom_t atom(const std::string &name, const std::string &atom,
                    bool exists);
    xcb_ewmh_connection_t *ewmh(const std::string &name);

private:
    void onConnectionCreated(XCBConnection &conn);
    void onConnectionClosed(XCBConnection &conn);

    Instance *instance_;
    std::unordered_map<std::string, XCBConnection> conns_;
    HandlerTable<XCBConnectionCreated> createdCallbacks_;
    HandlerTable<XCBConnectionClosed> closedCallbacks_;
    std::string mainDisplay_;
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addEventFilter);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addConnectionCreatedCallback);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addConnectionClosedCallback);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, xkbState);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, xkbRulesNames);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addSelection);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, convertSelection);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, atom);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, ewmh);
};
}

#endif // _FCITX_MODULES_XCB_XCBMODULE_H_
