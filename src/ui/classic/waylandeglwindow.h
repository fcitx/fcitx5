/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDEGLWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDEGLWINDOW_H_

#include "config.h"

#ifdef CAIRO_EGL_FOUND

#include <cairo/cairo.h>
#include <wayland-egl.h>
#include "waylandui.h"
#include "waylandwindow.h"

namespace fcitx {
namespace classicui {

class WaylandEGLWindow : public WaylandWindow {
public:
    WaylandEGLWindow(WaylandUI *ui);
    ~WaylandEGLWindow();

    void createWindow() override;
    void destroyWindow() override;

    cairo_surface_t *prerender() override;
    void render() override;
    void hide() override;

private:
    UniqueCPtr<wl_egl_window, wl_egl_window_destroy> window_;
    EGLSurface eglSurface_ = nullptr;
    UniqueCPtr<cairo_surface_t, cairo_surface_destroy> cairoSurface_;
    std::unique_ptr<wayland::WlCallback> callback_;
};
} // namespace classicui
} // namespace fcitx

#endif

#endif // _FCITX_UI_CLASSIC_WAYLANDEGLWINDOW_H_
