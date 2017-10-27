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

#include "xcbinputwindow.h"
#include "fcitx-utils/rect.h"
#include <pango/pangocairo.h>
#include <xcb/xcb_aux.h>

namespace fcitx {
namespace classicui {

XCBInputWindow::XCBInputWindow(XCBUI *ui)
    : XCBWindow(ui), InputWindow(ui->parent()) {
    cairo_hint_style_t hint = CAIRO_HINT_STYLE_DEFAULT;
    cairo_antialias_t aa = CAIRO_ANTIALIAS_DEFAULT;
    cairo_subpixel_order_t subpixel = CAIRO_SUBPIXEL_ORDER_DEFAULT;
    switch (ui->fontOption().hint) {
    case XCBHintStyle::None:
        hint = CAIRO_HINT_STYLE_NONE;
        break;
    case XCBHintStyle::Slight:
        hint = CAIRO_HINT_STYLE_SLIGHT;
        break;
    case XCBHintStyle::Medium:
        hint = CAIRO_HINT_STYLE_MEDIUM;
        break;
    case XCBHintStyle::Full:
        hint = CAIRO_HINT_STYLE_FULL;
        break;
    default:
        hint = CAIRO_HINT_STYLE_DEFAULT;
        break;
    }
    switch (ui->fontOption().rgba) {
    case XCBRGBA::None:
        subpixel = CAIRO_SUBPIXEL_ORDER_DEFAULT;
        break;
    case XCBRGBA::RGB:
        subpixel = CAIRO_SUBPIXEL_ORDER_RGB;
        break;
    case XCBRGBA::BGR:
        subpixel = CAIRO_SUBPIXEL_ORDER_BGR;
        break;
    case XCBRGBA::VRGB:
        subpixel = CAIRO_SUBPIXEL_ORDER_VRGB;
        break;
    case XCBRGBA::VBGR:
        subpixel = CAIRO_SUBPIXEL_ORDER_VBGR;
        break;
    default:
        subpixel = CAIRO_SUBPIXEL_ORDER_DEFAULT;
        break;
    }

    if (ui->fontOption().antialias) {
        if (subpixel != CAIRO_SUBPIXEL_ORDER_DEFAULT) {
            aa = CAIRO_ANTIALIAS_SUBPIXEL;
        } else {
            aa = CAIRO_ANTIALIAS_GRAY;
        }
    } else {
        aa = CAIRO_ANTIALIAS_NONE;
    }

    auto options = cairo_font_options_create();
    cairo_font_options_set_hint_style(options, hint);
    cairo_font_options_set_subpixel_order(options, subpixel);
    cairo_font_options_set_antialias(options, aa);
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_ON);
    pango_cairo_context_set_font_options(context_.get(), options);
    cairo_font_options_destroy(options);
}

void XCBInputWindow::postCreateWindow() {
    addEventMaskToWindow(
        ui_->connection(), wid_,
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE |
            XCB_EVENT_MASK_LEAVE_WINDOW);
}

void XCBInputWindow::updatePosition(InputContext *inputContext) {
    if (!visible()) {
        return;
    }
    int x, y, h;

    x = inputContext->cursorRect().left();
    y = inputContext->cursorRect().top();
    h = inputContext->cursorRect().height();

    const Rect *closestScreen = nullptr;
    int shortestDistance = INT_MAX;
    for (auto &rect : ui_->screenRects()) {
        int thisDistance = rect.first.distance(x, y);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = &rect.first;
        }
    }

    if (closestScreen) {
        int newX, newY;

        if (x < closestScreen->left()) {
            newX = closestScreen->left();
        } else {
            newX = x;
        }

        if (y < closestScreen->top()) {
            newY = closestScreen->top();
        } else {
            newY = y + (h ? h : (10 * ((dpi_ < 0 ? 96.0 : dpi_) / 96.0)));
        }

        if ((newX + static_cast<int>(width())) > closestScreen->right())
            newX = closestScreen->right() - width();

        if ((newY + static_cast<int>(height())) > closestScreen->bottom()) {
            if (newY > closestScreen->bottom())
                newY = closestScreen->bottom() - height() - 40;
            else { /* better position the window */
                newY = newY - height() - ((h == 0) ? 40 : h);
            }
        }
        x = newX;
        y = newY;
    }

    xcb_params_configure_window_t wc;
    wc.x = x;
    wc.y = y;
    wc.stack_mode = XCB_STACK_MODE_ABOVE;
    xcb_aux_configure_window(ui_->connection(), wid_,
                             XCB_CONFIG_WINDOW_STACK_MODE |
                                 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             &wc);
    xcb_flush(ui_->connection());
}

void XCBInputWindow::updateDPI(InputContext *inputContext) {
    int x, y;

    x = inputContext->cursorRect().left();
    y = inputContext->cursorRect().top();

    int shortestDistance = INT_MAX;
    int dpi = -1;
    for (auto &rect : ui_->screenRects()) {
        int thisDistance = rect.first.distance(x, y);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            dpi = rect.second;
        }
    }

    dpi_ = ui_->dpi(dpi);
}

void XCBInputWindow::update(InputContext *inputContext) {
    if (!wid_) {
        return;
    }
    auto oldVisible = visible();
    if (inputContext) {
        updateDPI(inputContext);
    }
    InputWindow::update(inputContext);
    if (!visible()) {
        if (oldVisible) {
            xcb_unmap_window(ui_->connection(), wid_);

            xcb_unmap_notify_event_t event;
            memset(&event, 0, sizeof(event));
            xcb_screen_t *screen =
                xcb_aux_get_screen(ui_->connection(), ui_->defaultScreen());
            event.response_type = XCB_UNMAP_NOTIFY;
            event.event = screen->root;
            event.window = wid_;
            event.from_configure = false;
            xcb_send_event(ui_->connection(), false, screen->root,
                           XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                               XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                           reinterpret_cast<const char *>(&event));
            xcb_flush(ui_->connection());
        }
        return;
    }
    auto pair = sizeHint();
    unsigned int width = pair.first, height = pair.second;

    if (width != this->width() || height != this->height()) {
        resize(width, height);
    }

    cairo_t *c = cairo_create(prerender());
    updatePosition(inputContext);
    if (!oldVisible) {
        xcb_map_window(ui_->connection(), wid_);
        xcb_flush(ui_->connection());
    }
    paint(c, width, height);
    cairo_destroy(c);
    render();
}

bool XCBInputWindow::filterEvent(xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    switch (response_type) {

    case XCB_EXPOSE: {
        auto expose = reinterpret_cast<xcb_expose_event_t *>(event);
        if (expose->window == wid_) {
            if (visible()) {
                if (auto surface = prerender()) {
                    cairo_t *c = cairo_create(surface);
                    paint(c, width(), height());
                    cairo_destroy(c);
                    render();
                }
            }
            return true;
        }
        break;
    }
    }
    return false;
}
}
}
