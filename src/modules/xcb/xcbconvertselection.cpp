/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "xcbconvertselection.h"
#include "xcbconnection.h"
#include "xcbmodule.h"

namespace fcitx {

XCBConvertSelectionRequest::XCBConvertSelectionRequest(
    XCBConnection *conn, xcb_atom_t selection, xcb_atom_t type,
    xcb_atom_t property, XCBConvertSelectionCallback callback)

    : conn_(conn), selection_(selection), property_(property),
      realCallback_(std::move(callback)) {
    if (type == 0) {
        fallbacks_.push_back(XCB_ATOM_STRING);
        auto utf8Atom = conn->atom("UTF8_STRING", true);
        if (utf8Atom) {
            fallbacks_.push_back(utf8Atom);
        }
    } else {
        fallbacks_.push_back(type);
    }
    xcb_delete_property(conn->connection(), conn->serverWindow(), property_);
    xcb_convert_selection(conn->connection(), conn->serverWindow(), selection_,
                          fallbacks_.back(), property_, XCB_TIME_CURRENT_TIME);
    xcb_flush(conn->connection());
    timer_ = conn->parent()->instance()->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 5000000, 0,
        [this](EventSourceTime *, uint64_t) {
            invokeCallbackAndCleanUp(XCB_ATOM_NONE, nullptr, 0);
            return true;
        });
}

void XCBConvertSelectionRequest::cleanUp() {
    realCallback_ = decltype(realCallback_)();
    timer_.reset();
}

void XCBConvertSelectionRequest::invokeCallbackAndCleanUp(xcb_atom_t type,
                                                          const char *data,
                                                          size_t length) {
    // Make a copy to real callback, because it might delete the this.
    auto realCallback = realCallback_;
    cleanUp();
    if (realCallback) {
        realCallback(type, data, length);
    }
}

void XCBConvertSelectionRequest::handleReply(xcb_atom_t type, const char *data,
                                             size_t length) {
    if (!realCallback_) {
        return;
    }
    if (type == fallbacks_.back()) {
        fallbacks_.pop_back();
        return invokeCallbackAndCleanUp(type, data, length);
    }

    fallbacks_.pop_back();
    if (fallbacks_.empty()) {
        return invokeCallbackAndCleanUp(XCB_ATOM_NONE, nullptr, 0);
    }

    xcb_delete_property(conn_->connection(), conn_->serverWindow(), property_);
    xcb_convert_selection(conn_->connection(), conn_->serverWindow(),
                          selection_, fallbacks_.back(), property_,
                          XCB_TIME_CURRENT_TIME);
    xcb_flush(conn_->connection());
}

} // namespace fcitx
