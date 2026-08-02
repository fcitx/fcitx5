/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "xcbwindow.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cairo-xcb.h>
#include <cairo.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/rect.h"
#include "common.h"
#include "window.h"
#include "xcb_public.h"
#include "xcbui.h"

namespace fcitx::classicui {

XCBWindow::XCBWindow(XCBUI *ui, int width, int height) : ui_(ui) {
    Window::resize(width, height);
}

XCBWindow::~XCBWindow() { destroyWindow(); }

void XCBWindow::createWindow(xcb_visualid_t vid, bool overrideRedirect) {
    auto *conn = ui_->connection();

    if (wid_) {
        destroyWindow();
    }
    xcb_screen_t *screen = xcb_aux_get_screen(conn, ui_->defaultScreen());

    xcb_colormap_t colorMap;
    CLASSICUI_DEBUG() << "Create window with vid: " << vid;
    if (vid == ui_->visualId()) {
        colorMap = ui_->colorMap();
        colorMapNeedFree_ = 0;
        CLASSICUI_DEBUG() << "Use shared color map: " << colorMap;
    } else if (vid) {
        colorMapNeedFree_ = xcb_generate_id(conn);
        xcb_create_colormap(conn, XCB_COLORMAP_ALLOC_NONE, colorMapNeedFree_,
                            screen->root, vid);
        colorMap = colorMapNeedFree_;
        CLASSICUI_DEBUG() << "Use new color map: " << colorMapNeedFree_;
    } else {
        colorMapNeedFree_ = 0;
        colorMap = XCB_COPY_FROM_PARENT;
        CLASSICUI_DEBUG() << "Use color map copy from parent";
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
    params.colormap = colorMap;
    vid_ = vid;

    physicalWidth_ = physicalSizeFromLogical(width_);
    physicalHeight_ = physicalSizeFromLogical(height_);

    auto cookie = xcb_aux_create_window_checked(
        conn, depth, wid_, screen->root, 0, 0, physicalWidth_, physicalHeight_,
        0, XCB_WINDOW_CLASS_INPUT_OUTPUT, vid, valueMask, &params);
    if (auto error = makeUniqueCPtr(xcb_request_check(conn, cookie))) {
        CLASSICUI_DEBUG() << "Create window failed: "
                          << static_cast<int>(error->error_code) << " " << vid
                          << " " << colorMap;
    } else {
        CLASSICUI_DEBUG() << "Window created id: " << wid_;
    }
    constexpr uint32_t XEMBED_VERSION = 0;
    constexpr uint32_t XEMBED_MAPPED = (1 << 0);
    uint32_t data[] = {XEMBED_VERSION, XEMBED_MAPPED};
    xcb_atom_t _XEMBED_INFO = ui_->parent()->xcb()->call<IXCBModule::atom>(
        ui_->displayName(), "_XEMBED_INFO", false);
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, wid_, _XEMBED_INFO,
                        _XEMBED_INFO, 32, 2, data);

    eventFilter_ = ui_->parent()->xcb()->call<IXCBModule::addEventFilter>(
        ui_->displayName(),
        [this](xcb_connection_t *, xcb_generic_event_t *event) {
            return filterEvent(event);
        });

    surface_.reset(cairo_xcb_surface_create(
        conn, wid_,
        vid ? xcb_aux_find_visual_by_id(screen, vid)
            : xcb_aux_find_visual_by_id(screen, screen->root_visual),
        physicalWidth_, physicalHeight_));
    if (surface_) {
        cairo_surface_set_device_scale(surface_.get(), scale_, scale_);
        ui_->setCairoDevice(cairo_surface_get_device(surface_.get()));
    }
    contentSurface_.reset();

    postCreateWindow();
}

void XCBWindow::destroyWindow() {
    auto *conn = ui_->connection();
    eventFilter_.reset();
    if (wid_) {
        xcb_destroy_window(conn, wid_);
        wid_ = 0;
    }
    if (colorMapNeedFree_) {
        xcb_free_colormap(conn, colorMapNeedFree_);
        colorMapNeedFree_ = 0;
    }

    if (ui_->pointerGrabber() == this) {
        ui_->ungrabPointer();
    }
}

void XCBWindow::setScale(double scale) {
    if (scale_ != scale && scale > 0) {
        scale_ = scale;
        cairo_surface_set_device_scale(surface_.get(), scale_, scale_);
    }
}

void XCBWindow::resize(unsigned int width, unsigned int height) {
    auto newPhysicalWidth = physicalSizeFromLogical(width);
    auto newPhysicalHeight = physicalSizeFromLogical(height);
    if (newPhysicalWidth != physicalWidth_ ||
        newPhysicalHeight != physicalHeight_) {
        const uint32_t vals[2] = {static_cast<uint32_t>(newPhysicalWidth),
                                  static_cast<uint32_t>(newPhysicalHeight)};
        xcb_configure_window(ui_->connection(), wid_,
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             vals);
        cairo_xcb_surface_set_size(surface_.get(), newPhysicalWidth,
                                   newPhysicalHeight);
        physicalWidth_ = newPhysicalWidth;
        physicalHeight_ = newPhysicalHeight;
    }
    Window::resize(width, height);
    CLASSICUI_DEBUG() << "Resize: " << width << " " << height << " scale "
                      << scale_ << " physical " << physicalWidth_ << " "
                      << physicalHeight_;
}

cairo_surface_t *XCBWindow::prerender() {
#if 1
    contentSurface_.reset(cairo_surface_create_similar_image(
        surface_.get(), CAIRO_FORMAT_ARGB32, physicalWidth_, physicalHeight_));
#else
    contentSurface_.reset(cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, physicalWidth_, physicalHeight_));
#endif
    cairo_surface_set_device_scale(contentSurface_.get(), scale_, scale_);
    return contentSurface_.get();
}

void XCBWindow::render() {
    auto *cr = cairo_create(surface_.get());
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, contentSurface_.get(), 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    CLASSICUI_DEBUG() << "Render";
}

double XCBWindow::logicalFromPhysical(double value) const {
    return std::floor(value / scale_);
}

double XCBWindow::physicalFromLogical(double value) const {
    return std::round(value * scale_);
}

int XCBWindow::physicalSizeFromLogical(double size) const {
    return std::max(1.0, physicalFromLogical(size));
}

Rect XCBWindow::physicalFromLogical(const Rect &rect) const {
    return Rect()
        .setLeft(physicalFromLogical(rect.left()))
        .setTop(physicalFromLogical(rect.top()))
        .setRight(physicalFromLogical(rect.right()))
        .setBottom(physicalFromLogical(rect.bottom()));
}

double XCBWindow::scaleForDPI(int dpi) {
    if (dpi <= 0) {
        return 1.0;
    }
    return static_cast<double>(dpi) / 96.0;
}

} // namespace fcitx::classicui
