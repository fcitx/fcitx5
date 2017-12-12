/*
* Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_MODULES_XCB_XCBKEYBOARD_H_
#define _FCITX_MODULES_XCB_XCBKEYBOARD_H_

#include "xcb_public.h"
#include <memory>
#include <string>
#include <vector>
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>

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
    XCBConnection *conn_;
    uint8_t xkbFirstEvent_ = 0;
    uint8_t xkbMajorOpCode_ = 0;
    int32_t coreDeviceId_ = 0;
    bool hasXKB_ = false;
    xcb_atom_t xkbRulesNamesAtom_ = XCB_ATOM_NONE;

    std::unique_ptr<struct xkb_context, decltype(&xkb_context_unref)> context_{
        nullptr, &xkb_context_unref};
    std::unique_ptr<struct xkb_keymap, decltype(&xkb_keymap_unref)> keymap_{
        nullptr, &xkb_keymap_unref};
    std::unique_ptr<struct xkb_state, decltype(&xkb_state_unref)> state_{
        nullptr, &xkb_state_unref};

    std::vector<std::string> defaultLayouts_;
    std::vector<std::string> defaultVariants_;
    std::string xkbRule_;
    std::string xkbModel_;
    std::string xkbOptions_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;

    std::unique_ptr<EventSourceTime> xmodmapTimer_;
    int lastSequence_ = 0;
    bool waitingForRefresh_ = false;
};

} // namespace fcitx

#endif // _FCITX_MODULES_XCB_XCBKEYBOARD_H_
