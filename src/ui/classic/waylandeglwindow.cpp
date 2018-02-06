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
#include "config.h"

#ifdef CAIRO_EGL_FOUND

#include "waylandeglwindow.h"
#include "wl_callback.h"
#include <cairo/cairo-gl.h>
#include <wayland-egl.h>

namespace fcitx {
namespace classicui {
WaylandEGLWindow::WaylandEGLWindow(WaylandUI *ui)
    : WaylandWindow(ui), window_(nullptr, &wl_egl_window_destroy),
      cairoSurface_(nullptr, &cairo_surface_destroy) {}

WaylandEGLWindow::~WaylandEGLWindow() { destroyWindow(); }

void WaylandEGLWindow::createWindow() { WaylandWindow::createWindow(); }

void WaylandEGLWindow::destroyWindow() {
    hide();
    WaylandWindow::destroyWindow();
}

cairo_surface_t *WaylandEGLWindow::prerender() {
    if (width_ == 0 || height_ == 0) {
        hide();
        return nullptr;
    }

    if (!window_) {
        window_.reset(wl_egl_window_create(*surface_, width_, height_));
    }
    if (window_ && !eglSurface_) {
        eglSurface_ = ui_->createEGLSurface(window_.get(), nullptr);
    }
    if (eglSurface_ && !cairoSurface_) {
        cairoSurface_.reset(
            ui_->createEGLCairoSurface(eglSurface_, width_, height_));
    }
    if (!cairoSurface_) {
        return nullptr;
    }
    int width, height;
    wl_egl_window_get_attached_size(window_.get(), &width, &height);
    if (width != static_cast<int>(width_) ||
        height != static_cast<int>(height_)) {
        wl_egl_window_resize(window_.get(), width_, height_, 0, 0);
    }
    cairo_gl_surface_set_size(cairoSurface_.get(), width_, height_);
    if (cairo_surface_status(cairoSurface_.get()) != CAIRO_STATUS_SUCCESS) {
        return nullptr;
    }

    return cairoSurface_.get();
}

void WaylandEGLWindow::render() {
    if (cairo_surface_status(cairoSurface_.get()) != CAIRO_STATUS_SUCCESS) {
        return;
    }
    cairo_gl_surface_swapbuffers(cairoSurface_.get());
    callback_.reset(surface_->frame());
    callback_->done().connect([this](uint32_t) { callback_.reset(); });
}

void WaylandEGLWindow::hide() {
    cairoSurface_.reset();
    if (eglSurface_) {
        ui_->destroyEGLSurface(eglSurface_);
        eglSurface_ = nullptr;
    }
    window_.reset();
    surface_->attach(nullptr, 0, 0);
    surface_->commit();
}
} // namespace classicui
} // namespace fcitx

#endif
