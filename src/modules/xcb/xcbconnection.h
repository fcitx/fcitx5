/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_XCB_XCBCONNECTION_H_
#define _FCITX_MODULES_XCB_XCBCONNECTION_H_

#include <string>
#include <fcitx/instance.h>
#include <xcb/xcb_keysyms.h>
#include "fcitx-utils/event.h"
#include "fcitx-utils/handlertable.h"
#include "xcb_public.h"

namespace fcitx {

class XCBModule;
class XCBConvertSelectionRequest;
class XCBKeyboard;
class XCBEventReader;

class XCBConnection {
public:
    XCBConnection(XCBModule *xcb, const std::string &name);
    ~XCBConnection();

    std::unique_ptr<HandlerTableEntry<XCBEventFilter>>
    addEventFilter(XCBEventFilter filter);
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
    addSelection(const std::string &selection,
                 XCBSelectionNotifyCallback callback);

    std::unique_ptr<HandlerTableEntryBase>
    convertSelection(const std::string &selection, const std::string &type,
                     XCBConvertSelectionCallback callback);

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

    void processEvent();

private:
    bool filterEvent(xcb_connection_t *conn, xcb_generic_event_t *event);
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
    UniqueCPtr<xcb_connection_t, xcb_disconnect> conn_;
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
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;

    HandlerTable<XCBEventFilter> filters_;
    // need to be clean up before filters_ destructs;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> filter_;
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
        compositeCallback_;

    xcb_ewmh_connection_t ewmh_;

    std::unique_ptr<XCBKeyboard> keyboard_;

    UniqueCPtr<xcb_key_symbols_t, xcb_key_symbols_free> syms_;
    size_t groupIndex_ = 0;
    KeyList forwardGroup_, backwardGroup_;
    bool doGrab_ = false;
    bool keyboardGrabbed_ = false;

    std::unique_ptr<XCBEventReader> reader_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_XCB_XCBCONNECTION_H_
