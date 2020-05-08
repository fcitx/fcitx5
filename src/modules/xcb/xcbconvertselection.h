/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_XCB_XCBCONVERTSELECTION_H_
#define _FCITX_MODULES_XCB_XCBCONVERTSELECTION_H_

#include <xcb/xcb.h>
#include "fcitx-utils/event.h"
#include "xcb_public.h"

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
