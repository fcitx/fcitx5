/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDPOINTER_H_
#define _FCITX_UI_CLASSIC_WAYLANDPOINTER_H_

#include <memory>
#include "wl_pointer.h"
#include "wl_seat.h"

namespace fcitx {
namespace classicui {

class WaylandWindow;

class WaylandPointer {
public:
    WaylandPointer(wayland::WlSeat *seat);

private:
    void initPointer();
    std::unique_ptr<wayland::WlPointer> pointer_;
    TrackableObjectReference<WaylandWindow> focus_;
    int focusX_, focusY_;
    ScopedConnection capConn_;
};

} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_WAYLANDPOINTER_H_
