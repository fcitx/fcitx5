/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "xcbwindow.h"
#include <cairo/cairo-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include "common.h"

namespace fcitx {
namespace classicui {

XCBWindow::XCBWindow(XCBUI *ui, int width, int height)
    : Window(), ui_(ui), surface_(nullptr, &cairo_surface_destroy),
      contentSurface_(nullptr, &cairo_surface_destroy) {
    Window::resize(width, height);
}

XCBWindow::~XCBWindow() { destroyWindow(); }

void XCBWindow::createWindow(xcb_visualid_t vid, bool overrideRedirect) {
    auto conn = ui_->connection();

    if (wid_) {
        destroyWindow();
    }
    xcb_screen_t *screen = xcb_aux_get_screen(conn, ui_->defaultScreen());

    if (!vid) {
        vid = ui_->visualId();
    } else {
        if (vid != ui_->visualId()) {
            colorMap_ = xcb_generate_id(conn);
            xcb_create_colormap(conn, XCB_COLORMAP_ALLOC_NONE, colorMap_,
                                screen->root, vid);
        }
    }

    wid_ = xcb_generate_id(conn);

    auto depth = xcb_aux_get_depth_of_visual(screen, vid);

    uint32_t valueMask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
                         XCB_CW_BIT_GRAVITY | XCB_CW_BACKING_STORE |
                         XCB_CW_OVERRIDE_REDIRECT | XCB_CW_SAVE_UNDER |
                         XCB_CW_COLORMAP;

    if (overrideRedirect) {
        valueMask |= XCB_CW_OVERRIDE_REDIRECT;
    }

    xcb_params_cw_t params;
    memset(&params, 0, sizeof(params));
    params.back_pixel = 0;
    params.border_pixel = 0;
    params.bit_gravity = XCB_GRAVITY_NORTH_WEST;
    params.backing_store = XCB_BACKING_STORE_WHEN_MAPPED;
    params.override_redirect = overrideRedirect ? 1 : 0;
    params.save_under = 1;
    params.colormap = colorMap_ ? colorMap_ : ui_->colorMap();
    vid_ = vid;

    auto cookie = xcb_aux_create_window_checked(
        conn, depth, wid_, screen->root, 0, 0, width_, height_, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, vid, valueMask, &params);
    if (auto error = xcb_request_check(conn, cookie)) {
        CLASSICUI_DEBUG() << static_cast<int>(error->error_code);
        free(error);
    } else {
        CLASSICUI_DEBUG() << "Window created id: " << wid_;
    }

    eventFilter_ = ui_->parent()->xcb()->call<IXCBModule::addEventFilter>(
        ui_->name(), [this](xcb_connection_t *, xcb_generic_event_t *event) {
            return filterEvent(event);
        });

    surface_.reset(cairo_xcb_surface_create(
        conn, wid_, xcb_aux_find_visual_by_id(screen, vid), width_, height_));
    contentSurface_.reset();

    postCreateWindow();
    xcb_flush(ui_->connection());
}

void XCBWindow::destroyWindow() {
    auto conn = ui_->connection();
    eventFilter_.reset();
    if (wid_) {
        xcb_destroy_window(conn, wid_);
        wid_ = 0;
    }
    if (colorMap_) {
        xcb_free_colormap(conn, colorMap_);
        colorMap_ = 0;
    }
    xcb_flush(conn);
}

void XCBWindow::resize(unsigned int width, unsigned int height) {
    const uint32_t vals[2] = {width, height};
    xcb_configure_window(ui_->connection(), wid_,
                         XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                         vals);
    xcb_flush(ui_->connection());
    cairo_xcb_surface_set_size(surface_.get(), width, height);
    Window::resize(width, height);
    CLASSICUI_DEBUG() << "Resize: " << width << " " << height;
}

cairo_surface_t *XCBWindow::prerender() {
#if 1
    contentSurface_.reset(cairo_surface_create_similar(
        surface_.get(), CAIRO_CONTENT_COLOR_ALPHA, width(), height()));
#else
    contentSurface_.reset(
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width(), height()));
#endif
    return contentSurface_.get();
}

void XCBWindow::render() {
    auto cr = cairo_create(surface_.get());
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, contentSurface_.get(), 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    xcb_flush(ui_->connection());
    CLASSICUI_DEBUG() << "Render";
}
} // namespace classicui
} // namespace fcitx
