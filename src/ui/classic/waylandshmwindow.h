/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDSHMWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDSHMWINDOW_H_

#include <cairo/cairo.h>
#include "buffer.h"
#include "waylandui.h"
#include "waylandwindow.h"
#include "wl_callback.h"
#include "wl_shm.h"

namespace fcitx {
namespace classicui {

class WaylandShmWindow : public WaylandWindow {
public:
    WaylandShmWindow(WaylandUI *ui);
    ~WaylandShmWindow();

    void destroyWindow() override;
    cairo_surface_t *prerender() override;
    void render() override;
    void hide() override;

private:
    void newBuffer();

    std::shared_ptr<wayland::WlShm> shm_;
    std::vector<std::unique_ptr<wayland::Buffer>> buffers_;
    // Pointer to the current buffer.
    wayland::Buffer *buffer_ = nullptr;
    bool pending_ = false;
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_WAYLANDSHMWINDOW_H_
