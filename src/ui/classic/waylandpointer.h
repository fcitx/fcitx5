//
// Copyright (C) 2020~2020 by CSSlayer
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
