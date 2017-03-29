/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "waylandeglwindow.h"
#include <wayland-egl.h>

namespace fcitx {
namespace classicui {
WaylandEGLWindow::WaylandEGLWindow(WaylandUI *ui, UserInterfaceComponent type)
    : WaylandWindow(ui, type), window_(nullptr, &wl_egl_window_destroy),
      cairoSurface_(nullptr, &cairo_surface_destroy) {}

void WaylandEGLWindow::createWindow() {
    WaylandWindow::createWindow();
    window_.reset(wl_egl_window_create(*surface_, width(), height()));
    eglSurface_ = ui_->createEGLSurface(window_.get(), nullptr);
    cairoSurface_.reset(
        ui_->createEGLCairoSurface(eglSurface_, width(), height()));
}

void WaylandEGLWindow::destroyWindow() {
    cairoSurface_.reset();
    ui_->destroyEGLSurface(eglSurface_);
    window_.reset();
    WaylandWindow::destroyWindow();
}

void WaylandEGLWindow::resize(unsigned int width, unsigned int height) {
    wl_egl_window_resize(window_.get(), width, height, 0, 0);
}
}
}
