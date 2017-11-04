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
#ifndef _FCITX_UI_CLASSIC_WAYLANDSHMWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDSHMWINDOW_H_

#include "buffer.h"
#include "waylandui.h"
#include "waylandwindow.h"
#include "wl_callback.h"
#include "wl_shm.h"
#include <cairo/cairo.h>

namespace fcitx {
namespace classicui {

class WaylandShmWindow : public WaylandWindow {
public:
    WaylandShmWindow(WaylandUI *ui);
    ~WaylandShmWindow();

    void destroyWindow() override;
    cairo_surface_t *prerender() override;
    void render() override;

private:
    void newBuffer();

    std::shared_ptr<wayland::WlShm> shm_;
    std::vector<std::unique_ptr<wayland::Buffer>> buffers_;
    // Pointer to the current buffer.
    wayland::Buffer *buffer_ = nullptr;
    bool pending_ = false;
};
}
}

#endif // _FCITX_UI_CLASSIC_WAYLANDSHMWINDOW_H_
