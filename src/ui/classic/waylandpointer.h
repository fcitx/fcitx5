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
#include "wl_touch.h"

namespace fcitx {
namespace classicui {

class WaylandWindow;

class WaylandPointer {
public:
    WaylandPointer(wayland::WlSeat *seat);

private:
    void initPointer();
    void initTouch();
    std::unique_ptr<wayland::WlPointer> pointer_;
    TrackableObjectReference<WaylandWindow> pointerFocus_;
    int pointerFocusX_ = 0, pointerFocusY_ = 0;
    std::unique_ptr<wayland::WlTouch> touch_;
    TrackableObjectReference<WaylandWindow> touchFocus_;
    int touchFocusX_ = 0, touchFocusY_ = 0;
    ScopedConnection capConn_;
};

} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_WAYLANDPOINTER_H_
