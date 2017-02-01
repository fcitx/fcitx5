/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#include "waylandoutput.h"
#include "fcitx-utils/macros.h"

namespace fcitx {

namespace classicui {

const struct wl_output_listener WaylandOutput::outputListener = {
    [](void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
       int32_t subpixel, const char *make, const char *model, int32_t transform) {
        return static_cast<WaylandOutput *>(data)->geometry(wl_output, x, y, physical_width, physical_height, subpixel,
                                                            make, model, transform);
    },
    [](void *data, struct wl_output *wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
        return static_cast<WaylandOutput *>(data)->mode(wl_output, flags, width, height, refresh);
    },
    [](void *data, struct wl_output *wl_output) { return static_cast<WaylandOutput *>(data)->done(wl_output); },
    [](void *data, struct wl_output *wl_output, int32_t factor) {
        return static_cast<WaylandOutput *>(data)->scale(wl_output, factor);
    }};

WaylandOutput::WaylandOutput(wl_output *output) : output_(output) {}

WaylandOutput::~WaylandOutput() { wl_output_destroy(output_); }

void WaylandOutput::geometry(struct wl_output *, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
                             int32_t subpixel, const char *make, const char *model, int32_t transform) {
    FCITX_UNUSED(physical_width);
    FCITX_UNUSED(physical_height);
    FCITX_UNUSED(subpixel);
    geometry_.setPosition(x, y);
    transform_ = static_cast<wl_output_transform>(transform);
    make_ = make;
    model_ = model;
}

void WaylandOutput::mode(struct wl_output *, uint32_t flags, int32_t width, int32_t height, int32_t) {
    if (flags & WL_OUTPUT_MODE_CURRENT) {
        geometry_.setSize(width, height);
    }
}

void WaylandOutput::done(struct wl_output *) {}

void WaylandOutput::scale(struct wl_output *, int32_t factor) { scale_ = factor; }
}
}
