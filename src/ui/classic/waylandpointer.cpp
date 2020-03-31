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

#include "waylandpointer.h"
#include "waylandwindow.h"
#include "wl_surface.h"

namespace fcitx {
namespace classicui {

WaylandPointer::WaylandPointer(wayland::WlSeat *seat) {
    capConn_ = seat->capabilities().connect([this, seat](uint32_t caps) {
        if ((caps & WL_SEAT_CAPABILITY_POINTER) && !pointer_) {
            pointer_.reset(seat->getPointer());
            initPointer();
        } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && pointer_) {
            pointer_.reset();
        }
    });
}

void WaylandPointer::initPointer() {
    pointer_->enter().connect([this](uint32_t, wayland::WlSurface *surface,
                                     wl_fixed_t sx, wl_fixed_t sy) {
        auto window = static_cast<WaylandWindow *>(surface->userData());
        if (!window) {
            return;
        }
        focus_ = window->watch();
        focusX_ = wl_fixed_to_int(sx);
        focusY_ = wl_fixed_to_int(sy);
        window->hover()(focusX_, focusY_);
    });
    pointer_->leave().connect([this](uint32_t, wayland::WlSurface *surface) {
        if (auto window = focus_.get()) {
            if (window->surface() == surface) {
                focus_.unwatch();
                window->leave()();
            }
        }
    });
    pointer_->motion().connect([this](uint32_t, wl_fixed_t sx, wl_fixed_t sy) {
        if (auto window = focus_.get()) {
            focusX_ = wl_fixed_to_int(sx);
            focusY_ = wl_fixed_to_int(sy);
            window->hover()(focusX_, focusY_);
        }
    });
    pointer_->button().connect(
        [this](uint32_t, uint32_t, uint32_t button, uint32_t state) {
            if (auto window = focus_.get()) {
                window->click()(focusX_, focusY_, button, state);
            }
        });
    pointer_->axis().connect([this](uint32_t, uint32_t axis, wl_fixed_t value) {
        if (auto window = focus_.get()) {
            window->axis()(focusX_, focusY_, axis, value);
        }
    });
}

} // namespace classicui
} // namespace fcitx
