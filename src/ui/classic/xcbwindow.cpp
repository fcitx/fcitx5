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

#include "xcbwindow.h"
#include <cairo/cairo-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

namespace fcitx {
namespace classicui {

XCBWindow::XCBWindow(XCBUI *ui, UserInterfaceComponent type)
    : Window(type), ui_(ui) {}

XCBWindow::~XCBWindow() { destroyWindow(); }

void XCBWindow::createWindow() {
    auto conn = ui_->connection();

    if (wid_) {
        destroyWindow();
    }

    xcb_screen_t *screen = xcb_aux_get_screen(conn, ui_->defaultScreen());
    wid_ = xcb_generate_id(conn);

    auto depth = xcb_aux_get_depth_of_visual(screen, ui_->visualId());

    uint32_t valueMask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
                         XCB_CW_BIT_GRAVITY | XCB_CW_BACKING_STORE |
                         XCB_CW_OVERRIDE_REDIRECT | XCB_CW_SAVE_UNDER |
                         XCB_CW_COLORMAP;

    uint32_t values[7] = {
        0, 0, XCB_GRAVITY_NORTH_WEST, XCB_BACKING_STORE_WHEN_MAPPED,
        1, 1, ui_->colorMap()};

    xcb_create_window(conn, depth, wid_, screen->root, 0, 0, 1, 1, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, ui_->visualId(), valueMask,
                      values);

    eventFilter_.reset(ui_->parent()->xcb()->call<IXCBModule::addEventFilter>(
        ui_->name(), [this](xcb_connection_t *, xcb_generic_event_t *event) {
            return filterEvent(event);
        }));

    surface_ = cairo_xcb_surface_create(
        conn, wid_, xcb_aux_find_visual_by_id(screen, ui_->visualId()), 1, 1);
}

void XCBWindow::destroyWindow() {
    auto conn = ui_->connection();
    if (surface_) {
        cairo_surface_destroy(surface_);
    }
    eventFilter_.reset();
    xcb_destroy_window(conn, wid_);
    wid_ = 0;
}

void XCBWindow::resize(unsigned int width, unsigned int height) {
    const uint32_t vals[2] = {width, height};
    xcb_configure_window(ui_->connection(), wid_,
                         XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                         vals);
    cairo_xcb_surface_set_size(surface_, width, height);
}
}
}
