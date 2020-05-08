/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_WAYLAND_CORE_BUFFER_H_
#define _FCITX_WAYLAND_CORE_BUFFER_H_

#include <memory>
#include <cairo/cairo.h>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"

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
} // namespace wayland
} // namespace fcitx

#endif // _FCITX_WAYLAND_CORE_BUFFER_H_
