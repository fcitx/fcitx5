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
#ifndef _FCITX_MODULES_XCB_XCBCONVERTSELECTION_H_
#define _FCITX_MODULES_XCB_XCBCONVERTSELECTION_H_

#include "fcitx-utils/event.h"
#include "xcb_public.h"
#include <xcb/xcb.h>

namespace fcitx {

class XCBConnection;

class XCBConvertSelectionRequest {
public:
    XCBConvertSelectionRequest() = default;
    XCBConvertSelectionRequest(XCBConnection *conn, xcb_atom_t selection,
                               xcb_atom_t type, xcb_atom_t property,
                               XCBConvertSelectionCallback callback);

    XCBConvertSelectionRequest(const XCBConvertSelectionRequest &) = delete;

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

} // namespace fcitx

#endif // _FCITX_MODULES_XCB_XCBCONVERTSELECTION_H_
