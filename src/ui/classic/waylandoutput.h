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
#ifndef _FCITX_UI_CLASSIC_WAYLANDOUTPUT_H_
#define _FCITX_UI_CLASSIC_WAYLANDOUTPUT_H_

#include <fcitx-utils/rect.h>
#include <string>
#include <wayland-client.h>

namespace fcitx {
namespace classicui {

class WaylandOutput {
public:
    WaylandOutput(wl_output *output);
    ~WaylandOutput();

private:
    void geometry(struct wl_output *wl_output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
                  int32_t subpixel, const char *make, const char *model, int32_t transform);
    void mode(struct wl_output *wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
    void done(struct wl_output *wl_output);
    void scale(struct wl_output *wl_output, int32_t factor);

private:
    static const wl_output_listener outputListener;
    wl_output *output_;
    Rect geometry_;
    wl_output_transform transform_ = WL_OUTPUT_TRANSFORM_NORMAL;
    int32_t scale_ = 1;
    std::string make_;
    std::string model_;
};
}
}

#endif // _FCITX_UI_CLASSIC_WAYLANDOUTPUT_H_
