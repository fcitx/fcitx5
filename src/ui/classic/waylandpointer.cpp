/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

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
