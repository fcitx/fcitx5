/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_

#include "fcitx-utils/rect.h"
#include "waylandui.h"
#include "window.h"
#include "wl_surface.h"
#include <cairo/cairo.h>

namespace fcitx {
namespace classicui {

class WaylandWindow : public Window {
public:
    WaylandWindow(WaylandUI *ui);
    ~WaylandWindow();

    virtual void createWindow();
    virtual void destroyWindow();

    void setScale(int32_t scale) { scale_ = scale; }
    void setTransform(wl_output_transform transform) { transform_ = transform; }
    void hide();
    wayland::WlSurface *surface() { return surface_.get(); }

    auto &repaint() { return repaint_; }

protected:
    WaylandUI *ui_;
    std::unique_ptr<wayland::WlSurface> surface_;
    std::list<fcitx::ScopedConnection> conns_;
    Signal<void()> repaint_;

    Rect serverAllocation_;
    Rect allocation_;

    int scale_ = 1;
    wl_output_transform transform_ = WL_OUTPUT_TRANSFORM_NORMAL;
};

void bufferToSurfaceSize(enum wl_output_transform buffer_transform,
                         int32_t buffer_scale, int32_t *width, int32_t *height);
}
}

#endif // _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_
