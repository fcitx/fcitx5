/*
 * Copyright (C) 2017~2017 by CSSlayer
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

#include "xcbinputwindow.h"
#include "fcitx-utils/rect.h"
#include <iostream>
#include <xcb/xcb_aux.h>

namespace fcitx {
namespace classicui {

XCBInputWindow::XCBInputWindow(XCBUI *ui)
    : XCBWindow(ui), InputWindow(ui->parent()) {}

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
    int dpi = -1;
    for (auto &rect : ui_->screenRects()) {
        int thisDistance = rect.first.distance(x, y);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = &rect.first;
            dpi = rect.second;
        }
    }

    // if dpi changed due to screen, resize.
    dpi = ui_->dpi(dpi);
    if (dpi != dpi_) {
        dpi_ = dpi;
        unsigned int width, height;
        std::tie(width, height) = sizeHint();

        if (width != this->width() || height != this->height()) {
            resize(width, height);
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

void XCBInputWindow::update(InputContext *inputContext) {
    if (!wid_) {
        return;
    }
    auto oldVisible = visible();
    InputWindow::update(inputContext);
    if (!visible()) {
        if (oldVisible) {
            xcb_unmap_window(ui_->connection(), wid_);
            xcb_flush(ui_->connection());
        }
        return;
    }
    unsigned int width, height;
    std::tie(width, height) = sizeHint();

    if (width != this->width() || height != this->height()) {
        resize(width, height);
    }

    cairo_t *c = cairo_create(prerender());
    // TODO
    cairo_set_source_rgba(c, 1, 1, 1, 1);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_paint(c);
    updatePosition(inputContext);
    if (!oldVisible) {
        xcb_map_window(ui_->connection(), wid_);
    }
    paint(c);
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
                    cairo_set_source_rgba(c, 1, 1, 1, 1);
                    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
                    cairo_paint(c);
                    paint(c);
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
