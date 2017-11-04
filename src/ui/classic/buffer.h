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
#ifndef _FCITX_WAYLAND_CORE_BUFFER_H_
#define _FCITX_WAYLAND_CORE_BUFFER_H_

#include "fcitx-utils/signals.h"
#include <cairo/cairo.h>
#include <memory>
#include <wayland-client.h>

namespace fcitx {
namespace wayland {

class WlShm;
class WlShmPool;
class WlBuffer;
class WlCallback;
class WlSurface;

class Buffer {
public:
    Buffer(WlShm *shm, uint32_t width, uint32_t height, wl_shm_format format);
    ~Buffer();

    bool busy() const { return busy_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    cairo_surface_t *cairoSurface() const { return surface_.get(); }
    WlBuffer *buffer() const { return buffer_.get(); }

    void attachToSurface(WlSurface *surface);

    auto &rendered() { return rendered_; }

private:
    Signal<void()> rendered_;
    std::unique_ptr<WlShmPool> pool_;
    std::unique_ptr<WlBuffer> buffer_;
    std::unique_ptr<WlCallback> callback_;
    std::unique_ptr<cairo_surface_t, decltype(&cairo_surface_destroy)> surface_;
    bool busy_ = false;
    uint32_t width_, height_;
};
}
}

#endif // _FCITX_WAYLAND_CORE_BUFFER_H_
