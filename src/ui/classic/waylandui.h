/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDUI_H_
#define _FCITX_UI_CLASSIC_WAYLANDUI_H_

#include <cairo/cairo.h>
#include "classicui.h"
#include "config.h"
#include "display.h"
#include "waylandpointer.h"
#include "wl_pointer.h"

#ifdef CAIRO_EGL_FOUND

#include <EGL/egl.h>
#include <wayland-egl.h>

#endif

namespace fcitx {
namespace classicui {

class WaylandWindow;
class WaylandInputWindow;

class WaylandUI : public UIInterface {
public:
    WaylandUI(ClassicUI *parent, const std::string &name, wl_display *display);
    ~WaylandUI();

#ifdef CAIRO_EGL_FOUND

    bool initEGL();
    EGLSurface createEGLSurface(wl_egl_window *window,
                                const EGLint *attrib_list);
    void destroyEGLSurface(EGLSurface surface);
    EGLContext argbCtx() { return argbCtx_; }
    EGLDisplay eglDisplay() { return eglDisplay_; }

    cairo_surface_t *createEGLCairoSurface(EGLSurface surface, int width,
                                           int height);
#endif

    ClassicUI *parent() const { return parent_; }
    const std::string &name() const { return name_; }
    wayland::Display *display() const { return display_; }
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;
    void suspend() override;
    void resume() override;
    void setEnableTray(bool) override {}

    std::unique_ptr<WaylandWindow> newWindow();

private:
    void setupInputWindow();

    ClassicUI *parent_;
    std::string name_;
    wayland::Display *display_;
    ScopedConnection panelConn_, panelRemovedConn_;
    std::unique_ptr<WaylandPointer> pointer_;
    std::unique_ptr<WaylandInputWindow> inputWindow_;

    bool isSuspend_ = true;
    bool hasEgl_ = false;
#ifdef CAIRO_EGL_FOUND
    // EGL stuff
    EGLDisplay eglDisplay_ = nullptr;
    EGLConfig argbConfig_ = nullptr;
    EGLContext argbCtx_ = nullptr;
    cairo_device_t *argbDevice_ = nullptr;
#endif
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_WAYLANDUI_H_
