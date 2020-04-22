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
    std::unique_ptr<wl_egl_window, decltype(&wl_egl_window_destroy)> window_;
    EGLSurface eglSurface_ = nullptr;
    std::unique_ptr<cairo_surface_t, decltype(&cairo_surface_destroy)>
        cairoSurface_;
    std::unique_ptr<wayland::WlCallback> callback_;
};
} // namespace classicui
} // namespace fcitx

#endif

#endif // _FCITX_UI_CLASSIC_WAYLANDEGLWINDOW_H_
