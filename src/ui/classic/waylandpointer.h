/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDPOINTER_H_
#define _FCITX_UI_CLASSIC_WAYLANDPOINTER_H_

#include <memory>
#include "waylandcursor.h"
#include "wl_pointer.h"
#include "wl_seat.h"
#include "wl_touch.h"

namespace fcitx {
namespace classicui {

class WaylandUI;
class WaylandWindow;

class WaylandPointer {
public:
    WaylandPointer(WaylandUI *ui, wayland::WlSeat *seat);

    auto ui() const { return ui_; }
    auto pointer() const { return pointer_.get(); }
    auto enterSerial() const { return enterSerial_; }

    WaylandCursor *cursor() {
        if (!cursor_) {
            cursor_ = std::make_unique<WaylandCursor>(this);
        }
        return cursor_.get();
    }

private:
    void initPointer();
    void initTouch();
    WaylandUI *ui_;
    wayland::Display *display_;
    std::unique_ptr<wayland::WlPointer> pointer_;
    TrackableObjectReference<WaylandWindow> pointerFocus_;
    int pointerFocusX_ = 0, pointerFocusY_ = 0;
    std::unique_ptr<wayland::WlTouch> touch_;
    TrackableObjectReference<WaylandWindow> touchFocus_;
    int touchFocusX_ = 0, touchFocusY_ = 0;
    ScopedConnection capConn_;
    uint32_t enterSerial_ = 0;
    std::unique_ptr<WaylandCursor> cursor_;
};

} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_WAYLANDPOINTER_H_
