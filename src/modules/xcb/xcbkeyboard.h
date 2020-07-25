/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_XCB_XCBKEYBOARD_H_
#define _FCITX_MODULES_XCB_XCBKEYBOARD_H_

#include <memory>
#include <string>
#include <vector>
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>
#include "xcb_public.h"

namespace fcitx {

class XCBConnection;

class XCBKeyboard {
public:
    XCBKeyboard(XCBConnection *connection);

    xcb_connection_t *connection();
    void updateKeymap();

    // Layout handling
    void initDefaultLayout();
    int findLayoutIndex(const std::string &layout, const std::string &variant);
    int findOrAddLayout(const std::string &layout, const std::string &variant,
                        bool toDefault);
    void addNewLayout(const std::string &layout, const std::string &variant,
                      int index, bool toDefault);
    void setRMLVOToServer(const std::string &rule, const std::string &model,
                          const std::string &layout, const std::string &variant,
                          const std::string &options);
    bool setLayoutByName(const std::string &layout, const std::string &variant,
                         bool toDefault);

    bool handleEvent(xcb_generic_event_t *event);
    XkbRulesNames xkbRulesNames();
    struct xkb_state *xkbState() {
        return state_.get();
    }

private:
    xcb_atom_t xkbRulesNamesAtom();
    XCBConnection *conn_;
    uint8_t xkbFirstEvent_ = 0;
    uint8_t xkbMajorOpCode_ = 0;
    int32_t coreDeviceId_ = 0;
    bool hasXKB_ = false;
    xcb_atom_t xkbRulesNamesAtom_ = XCB_ATOM_NONE;

    UniqueCPtr<struct xkb_context, xkb_context_unref> context_;
    UniqueCPtr<struct xkb_keymap, xkb_keymap_unref> keymap_;
    UniqueCPtr<struct xkb_state, xkb_state_unref> state_;

    std::vector<std::string> defaultLayouts_;
    std::vector<std::string> defaultVariants_;
    std::string xkbRule_;
    std::string xkbModel_;
    std::string xkbOptions_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;

    std::unique_ptr<EventSourceTime> updateKeymapEvent_;
    std::unique_ptr<EventSourceTime> xmodmapTimer_;
    int lastSequence_ = 0;
    bool waitingForRefresh_ = false;
};

} // namespace fcitx

#endif // _FCITX_MODULES_XCB_XCBKEYBOARD_H_
