/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "config.h"

#ifdef CAIRO_EGL_FOUND

#include <cairo-gl.h>
#include <wayland-egl.h>
#include "waylandeglwindow.h"
#include "wl_callback.h"

namespace fcitx {
namespace classicui {
WaylandEGLWindow::WaylandEGLWindow(WaylandUI *ui)
    : WaylandWindow(ui), window_(nullptr), cairoSurface_(nullptr) {}

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

    auto bufferWidth = width_ * scale_;
    auto bufferHeight = height_ * scale_;
    if (!window_) {
        window_.reset(
            wl_egl_window_create(*surface_, bufferWidth, bufferHeight));
    }
    if (window_ && !eglSurface_) {
        eglSurface_ = ui_->createEGLSurface(window_.get(), nullptr);
    }
    if (eglSurface_ && !cairoSurface_) {
        cairoSurface_.reset(
            ui_->createEGLCairoSurface(eglSurface_, bufferWidth, bufferHeight));
    }
    if (!cairoSurface_) {
        return nullptr;
    }
    int width, height;
    wl_egl_window_get_attached_size(window_.get(), &width, &height);
    if (width != static_cast<int>(bufferWidth) ||
        height != static_cast<int>(bufferHeight)) {
        wl_egl_window_resize(window_.get(), bufferWidth, bufferHeight, 0, 0);
    }
    cairo_gl_surface_set_size(cairoSurface_.get(), bufferWidth, bufferHeight);
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
    surface_->setBufferScale(scale_);
    surface_->commit();
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
