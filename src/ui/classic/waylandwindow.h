/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_

#include <cairo/cairo.h>
#include "fcitx-utils/rect.h"
#include "waylandui.h"
#include "window.h"
#include "wl_surface.h"

namespace fcitx {
namespace classicui {

class WaylandWindow : public Window, public TrackableObject<WaylandWindow> {
public:
    WaylandWindow(WaylandUI *ui);
    ~WaylandWindow();

    virtual void createWindow();
    virtual void destroyWindow();
    virtual void hide() = 0;

    void setScale(int32_t scale) { scale_ = scale; }
    void setTransform(wl_output_transform transform) { transform_ = transform; }
    wayland::WlSurface *surface() { return surface_.get(); }

    auto &repaint() { return repaint_; }
    auto &hover() { return hover_; }
    auto &click() { return click_; }
    auto &axis() { return axis_; }
    auto &leave() { return leave_; }

    auto &touchDown() { return touchDown_; }
    auto &touchUp() { return touchUp_; }

protected:
    WaylandUI *ui_;
    std::unique_ptr<wayland::WlSurface> surface_;
    std::list<fcitx::ScopedConnection> conns_;
    Signal<void()> repaint_;
    Signal<void(int, int)> hover_;
    Signal<void(int, int, uint32_t, uint32_t)> click_;
    Signal<void(int, int, uint32_t, wl_fixed_t)> axis_;
    Signal<void()> leave_;

    Signal<void(int, int)> touchDown_;
    Signal<void(int, int)> touchUp_;

    Rect serverAllocation_;
    Rect allocation_;

    int scale_ = 1;
    wl_output_transform transform_ = WL_OUTPUT_TRANSFORM_NORMAL;
};

void bufferToSurfaceSize(enum wl_output_transform buffer_transform,
                         int32_t buffer_scale, int32_t *width, int32_t *height);
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_
