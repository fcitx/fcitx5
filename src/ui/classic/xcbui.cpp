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

#include "xcbui.h"
#include <xcb/xcb_aux.h>

namespace fcitx {
namespace classicui {

xcb_visualid_t findVisual(xcb_screen_t *screen) {
    auto visual = xcb_aux_find_visual_by_attrs(screen, -1, 32);
    if (!visual) {
        return screen->root_visual;
    }
    return visual->visual_id;
}

XCBUI::XCBUI(ClassicUI *parent, const std::string &name, xcb_connection_t *conn,
             int defaultScreen)
    : parent_(parent), name_(name), conn_(conn), defaultScreen_(defaultScreen) {
    xcb_screen_t *screen = xcb_aux_get_screen(conn, defaultScreen);
    visualId_ = findVisual(screen);
    colorMap_ = xcb_generate_id(conn);
    xcb_create_colormap(conn, XCB_COLORMAP_ALLOC_NONE, colorMap_, screen->root,
                        visualId_);
}
}
}
