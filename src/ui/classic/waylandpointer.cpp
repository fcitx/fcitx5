/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandpointer.h"
#include "waylandwindow.h"
#include "wl_surface.h"

namespace fcitx::classicui {

WaylandPointer::WaylandPointer(wayland::WlSeat *seat) {
    capConn_ = seat->capabilities().connect([this, seat](uint32_t caps) {
        if ((caps & WL_SEAT_CAPABILITY_POINTER) && !pointer_) {
            pointer_.reset(seat->getPointer());
            initPointer();
        } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && pointer_) {
            pointer_.reset();
        }

        if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !touch_) {
            touch_.reset(seat->getTouch());
            initTouch();
        } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && touch_) {
            touch_.reset();
        }
    });
}

void WaylandPointer::initPointer() {
    pointer_->enter().connect([this](uint32_t, wayland::WlSurface *surface,
                                     wl_fixed_t sx, wl_fixed_t sy) {
        auto *window = static_cast<WaylandWindow *>(surface->userData());
        if (!window) {
            return;
        }
        pointerFocus_ = window->watch();
        pointerFocusX_ = wl_fixed_to_int(sx);
        pointerFocusY_ = wl_fixed_to_int(sy);
        window->hover()(pointerFocusX_, pointerFocusY_);
    });
    pointer_->leave().connect([this](uint32_t, wayland::WlSurface *surface) {
        if (auto *window = pointerFocus_.get()) {
            if (window->surface() == surface) {
                pointerFocus_.unwatch();
                window->leave()();
            }
        }
    });
    pointer_->motion().connect([this](uint32_t, wl_fixed_t sx, wl_fixed_t sy) {
        if (auto *window = pointerFocus_.get()) {
            pointerFocusX_ = wl_fixed_to_int(sx);
            pointerFocusY_ = wl_fixed_to_int(sy);
            window->hover()(pointerFocusX_, pointerFocusY_);
        }
    });
    pointer_->button().connect(
        [this](uint32_t, uint32_t, uint32_t button, uint32_t state) {
            if (auto *window = pointerFocus_.get()) {
                window->click()(pointerFocusX_, pointerFocusY_, button, state);
            }
        });
    pointer_->axis().connect([this](uint32_t, uint32_t axis, wl_fixed_t value) {
        if (auto *window = pointerFocus_.get()) {
            window->axis()(pointerFocusX_, pointerFocusY_, axis, value);
        }
    });
}

void WaylandPointer::initTouch() {
    touch_->down().connect([this](uint32_t, uint32_t,
                                  wayland::WlSurface *surface, int,
                                  wl_fixed_t sx, wl_fixed_t sy) {
        // TODO handle id for multiple touch
        auto *window = static_cast<WaylandWindow *>(surface->userData());
        if (!window) {
            return;
        }
        touchFocus_ = window->watch();
        touchFocusX_ = wl_fixed_to_int(sx);
        touchFocusY_ = wl_fixed_to_int(sy);
        window->touchDown()(touchFocusX_, touchFocusY_);
    });
    touch_->up().connect([this](uint32_t, uint32_t, int) {
        // TODO handle id for multiple touch
        if (auto *window = touchFocus_.get()) {
            window->touchUp()(touchFocusX_, touchFocusY_);
            touchFocus_.unwatch();
            window->leave()();
        }
    });
}

} // namespace fcitx::classicui
