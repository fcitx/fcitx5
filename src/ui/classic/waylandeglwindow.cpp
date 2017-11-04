/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#include "config.h"

#ifdef CAIRO_EGL_FOUND

#include "waylandeglwindow.h"
#include <cairo/cairo-gl.h>
#include <wayland-egl.h>

namespace fcitx {
namespace classicui {
WaylandEGLWindow::WaylandEGLWindow(WaylandUI *ui)
    : WaylandWindow(ui), window_(nullptr, &wl_egl_window_destroy),
      cairoSurface_(nullptr, &cairo_surface_destroy) {}

WaylandEGLWindow::~WaylandEGLWindow() { destroyWindow(); }

void WaylandEGLWindow::createWindow() {
    WaylandWindow::createWindow();
    window_.reset(wl_egl_window_create(*surface_, width(), height()));
    eglSurface_ = ui_->createEGLSurface(window_.get(), nullptr);
    cairoSurface_.reset(
        ui_->createEGLCairoSurface(eglSurface_, width(), height()));
}

void WaylandEGLWindow::destroyWindow() {
    cairoSurface_.reset();
    if (eglSurface_) {
        ui_->destroyEGLSurface(eglSurface_);
        eglSurface_ = nullptr;
    }
    window_.reset();
    WaylandWindow::destroyWindow();
}

void WaylandEGLWindow::resize(unsigned int width, unsigned int height) {
    wl_egl_window_resize(window_.get(), width, height, 0, 0);
    Window::resize(width, height);
}

cairo_surface_t *WaylandEGLWindow::prerender() {
    cairo_device_t *device = cairo_surface_get_device(cairoSurface_.get());
    if (!device) {
        return nullptr;
    }

    auto ctx = ui_->argbCtx();

    cairo_device_flush(device);
    cairo_device_acquire(device);
    eglMakeCurrent(ui_->eglDisplay(), eglSurface_, eglSurface_, ctx);

    return cairoSurface_.get();
}

void WaylandEGLWindow::render() {
    cairo_gl_surface_swapbuffers(cairoSurface_.get());
    int width, height;
    wl_egl_window_get_attached_size(window_.get(), &width, &height);

    bufferToSurfaceSize(transform_, scale_, &width, &height);
    serverAllocation_.setSize(width, height);

    cairo_device_t *device = cairo_surface_get_device(cairoSurface_.get());
    if (!device)
        return;

    eglMakeCurrent(ui_->eglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);
    cairo_device_release(device);
}
}
}

#endif
