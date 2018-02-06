//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_MODULES_XCB_XCBCONNECTION_H_
#define _FCITX_MODULES_XCB_XCBCONNECTION_H_

#include "fcitx-utils/event.h"
#include "fcitx-utils/handlertable.h"
#include "xcb_public.h"
#include <fcitx/instance.h>
#include <string>
#include <xcb/xcb_keysyms.h>

namespace fcitx {

class XCBModule;
class XCBConvertSelectionRequest;
class XCBKeyboard;

class XCBConnection {
public:
    XCBConnection(XCBModule *xcb, const std::string &name);
    ~XCBConnection();

    std::unique_ptr<HandlerTableEntry<XCBEventFilter>>
    addEventFilter(XCBEventFilter filter);
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
    addSelection(const std::string &name, XCBSelectionNotifyCallback callback);

    std::unique_ptr<HandlerTableEntryBase>
    convertSelection(const std::string &selection, const std::string &type,
                     XCBConvertSelectionCallback);

    XCBModule *parent() { return parent_; }
    Instance *instance();
    const std::string &name() const { return name_; }
    xcb_connection_t *connection() const { return conn_.get(); }
    int screen() const { return screen_; }
    FocusGroup *focusGroup() const { return group_; }
    xcb_window_t root() const { return root_; }
    struct xkb_state *xkbState();
    XkbRulesNames xkbRulesNames();
    xcb_window_t serverWindow() const { return serverWindow_; }

    void convertSelectionRequest(const XCBConvertSelectionRequest &request);
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
    std::unique_ptr<xcb_connection_t, decltype(&xcb_disconnect)> conn_{
        nullptr, &xcb_disconnect};
    int screen_ = 0;
    xcb_atom_t atom_ = XCB_ATOM_NONE;
    xcb_window_t serverWindow_ = XCB_WINDOW_NONE;
    xcb_window_t root_ = XCB_WINDOW_NONE;
    FocusGroup *group_ = nullptr;

    bool hasXFixes_ = false;
    uint8_t xfixesFirstEvent_ = 0;
    MultiHandlerTable<xcb_atom_t, XCBSelectionNotifyCallback> selections_{
        [this](xcb_atom_t selection) {
            addSelectionAtom(selection);
            return true;
        },
        [this](xcb_atom_t selection) { removeSelectionAtom(selection); }};

    HandlerTable<XCBConvertSelectionRequest> convertSelections_;

    std::unique_ptr<EventSourceIO> ioEvent_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;

    HandlerTable<XCBEventFilter> filters_;
    // need to be clean up before filters_ destructs;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> filter_;
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
        compositeCallback_;

    xcb_ewmh_connection_t ewmh_;

    std::unique_ptr<XCBKeyboard> keyboard_;

    std::unique_ptr<xcb_key_symbols_t, decltype(&xcb_key_symbols_free)> syms_{
        nullptr, &xcb_key_symbols_free};
    size_t groupIndex_ = 0;
    KeyList forwardGroup_, backwardGroup_;
    bool doGrab_ = false;
    bool keyboardGrabbed_ = false;
};

} // namespace fcitx

#endif // _FCITX_MODULES_XCB_XCBCONNECTION_H_
