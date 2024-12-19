/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "xcbinputwindow.h"
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include "fcitx-utils/rect.h"

namespace fcitx::classicui {

XCBInputWindow::XCBInputWindow(XCBUI *ui)
    : XCBWindow(ui), InputWindow(ui->parent()),
      atomBlur_(ui_->parent()->xcb()->call<IXCBModule::atom>(
          ui_->displayName(), "_KDE_NET_WM_BLUR_BEHIND_REGION", false)) {}

void XCBInputWindow::postCreateWindow() {
    if (ui_->ewmh()->_NET_WM_WINDOW_TYPE_COMBO &&
        ui_->ewmh()->_NET_WM_WINDOW_TYPE) {
        xcb_ewmh_set_wm_window_type(ui_->ewmh(), wid_, 1,
                                    &ui_->ewmh()->_NET_WM_WINDOW_TYPE_COMBO);
    }

    if (ui_->ewmh()->_NET_WM_PID) {
        xcb_ewmh_set_wm_pid(ui_->ewmh(), wid_, getpid());
    }

    const char name[] = "Fcitx5 Input Window";
    xcb_icccm_set_wm_name(ui_->connection(), wid_, XCB_ATOM_STRING, 8,
                          sizeof(name) - 1, name);
    const char klass[] = "fcitx\0fcitx";
    xcb_icccm_set_wm_class(ui_->connection(), wid_, sizeof(klass) - 1, klass);
    addEventMaskToWindow(
        ui_->connection(), wid_,
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE |
            XCB_EVENT_MASK_LEAVE_WINDOW);
}

const Rect *XCBInputWindow::getClosestScreen(const Rect &cursorRect) const {
    const Rect *closestScreen = nullptr;

    int shortestDistance = INT_MAX;
    for (const auto &rect : ui_->screenRects()) {
        int thisDistance =
            rect.first.distance(cursorRect.left(), cursorRect.top());
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = &rect.first;
        }
    }

    return closestScreen;
}

int XCBInputWindow::calculatePositionX(const Rect &cursorRect,
                                       const Rect *closestScreen) const {
    // TODO: RTL support.
    int x = cursorRect.left();

    auto &theme = parent_->theme();
    int leftSW = theme.inputPanel->shadowMargin->marginLeft.value();
    int rightSW = theme.inputPanel->shadowMargin->marginRight.value();

    int actualWidth = width() - leftSW - rightSW;
    actualWidth = actualWidth <= 0 ? width() : actualWidth;

    if (closestScreen != nullptr) {
        int newX = std::max(x, closestScreen->left());

        if (newX + actualWidth > closestScreen->right()) {
            newX = closestScreen->right() - actualWidth;
        }

        x = std::max(newX, closestScreen->left());
    }

    // exclude shadow border width
    x -= leftSW;

    return x;
}

int XCBInputWindow::calculatePositionY(const Rect &cursorRect,
                                       const Rect *closestScreen) const {
    // TODO: RTL support.
    int y = cursorRect.top();

    auto &theme = parent_->theme();
    int topSW = theme.inputPanel->shadowMargin->marginTop.value();
    int bottomSW = theme.inputPanel->shadowMargin->marginBottom.value();

    int actualHeight = height() - topSW - bottomSW;
    actualHeight = actualHeight <= 0 ? height() : actualHeight;

    if (closestScreen != nullptr) {
        int h = cursorRect.height();
        int newY;
        if (y < closestScreen->top()) {
            newY = closestScreen->top();
        } else {
            newY = y + (h ? h : (10 * ((dpi_ < 0 ? 96.0 : dpi_) / 96.0)));
        }

        // Try flip y.
        if (newY + actualHeight > closestScreen->bottom()) {
            if (newY > closestScreen->bottom()) {
                newY = closestScreen->bottom() - actualHeight - 40;
            } else { /* better position the window */
                newY = newY - actualHeight - ((h == 0) ? 40 : h);
            }

            // If after flip, top is out of the screen, we still prefer the top
            // edge to be always with in screen.
            if (newY < closestScreen->top()) {
                newY = closestScreen->top();
            }
        }

        y = newY;
    }

    // exclude shadow border width
    y -= topSW;

    return y;
}

void XCBInputWindow::updatePosition(InputContext *inputContext) {
    if (!visible()) {
        return;
    }

    const Rect &cursorRect = inputContext->cursorRect();
    const Rect *closestScreen = getClosestScreen(cursorRect);
    xcb_params_configure_window_t wc;
    wc.x = calculatePositionX(cursorRect, closestScreen);
    wc.y = calculatePositionY(cursorRect, closestScreen);
    wc.stack_mode = XCB_STACK_MODE_ABOVE;
    xcb_aux_configure_window(ui_->connection(), wid_,
                             XCB_CONFIG_WINDOW_STACK_MODE |
                                 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             &wc);
}

void XCBInputWindow::updateDPI(InputContext *inputContext) {
    dpi_ = ui_->dpiByPosition(inputContext->cursorRect().left(),
                              inputContext->cursorRect().top());

    setFontDPI(dpi_);
}

void XCBInputWindow::update(InputContext *inputContext) {
    if (!wid_) {
        return;
    }
    auto oldVisible = visible();
    if (inputContext) {
        updateDPI(inputContext);
    }
    auto [width, height] = InputWindow::update(inputContext);
    if (!visible()) {
        if (oldVisible) {
            xcb_unmap_window(ui_->connection(), wid_);
            hoverIndex_ = -1;
        }
        return;
    }

    if (width != this->width() || height != this->height()) {
        resize(width, height);
        if (atomBlur_) {
            Rect rect(0, 0, width, height);
            shrink(rect, *ui_->parent()->theme().inputPanel->blurMargin);
            if (!*ui_->parent()->theme().inputPanel->enableBlur ||
                rect.isEmpty()) {
                xcb_delete_property(ui_->connection(), wid_, atomBlur_);
            } else {
                std::vector<uint32_t> data;
                if (ui_->parent()->theme().inputPanel->blurMask->empty()) {
                    data.push_back(rect.left());
                    data.push_back(rect.top());
                    data.push_back(rect.width());
                    data.push_back(rect.height());
                    xcb_change_property(ui_->connection(),
                                        XCB_PROP_MODE_REPLACE, wid_, atomBlur_,
                                        XCB_ATOM_CARDINAL, 32, data.size(),
                                        data.data());
                } else {
                    auto region = parent_->theme().mask(
                        parent_->theme().maskConfig(), width, height);
                    for (const auto &rect : region) {
                        data.push_back(rect.left());
                        data.push_back(rect.top());
                        data.push_back(rect.width());
                        data.push_back(rect.height());
                    }
                    xcb_change_property(ui_->connection(),
                                        XCB_PROP_MODE_REPLACE, wid_, atomBlur_,
                                        XCB_ATOM_CARDINAL, 32, data.size(),
                                        data.data());
                }
            }
        }
    }

    cairo_t *c = cairo_create(prerender());
    updatePosition(inputContext);
    if (!oldVisible) {
        xcb_map_window(ui_->connection(), wid_);
    }
    paint(c, width, height, /*scale=*/1.0);
    cairo_destroy(c);
    render();
}

bool XCBInputWindow::filterEvent(xcb_generic_event_t *event) {
    uint8_t response_type = event->response_type & ~0x80;
    switch (response_type) {

    case XCB_EXPOSE: {
        auto *expose = reinterpret_cast<xcb_expose_event_t *>(event);
        if (expose->window == wid_) {
            repaint();
            return true;
        }
        break;
    }
    case XCB_BUTTON_PRESS: {
        auto *buttonPress = reinterpret_cast<xcb_button_press_event_t *>(event);
        if (buttonPress->event != wid_) {
            break;
        }
        if (buttonPress->detail == XCB_BUTTON_INDEX_1) {
            click(buttonPress->event_x, buttonPress->event_y);
        } else if (buttonPress->detail == XCB_BUTTON_INDEX_4) {
            wheel(/*up=*/true);
        } else if (buttonPress->detail == XCB_BUTTON_INDEX_5) {
            wheel(/*up=*/false);
        }
        return true;
    }
    case XCB_MOTION_NOTIFY: {
        auto *motion = reinterpret_cast<xcb_motion_notify_event_t *>(event);
        if (motion->event == wid_) {
            if (hover(motion->event_x, motion->event_y)) {
                repaint();
            }
            return true;
        }
        break;
    }
    case XCB_LEAVE_NOTIFY: {
        auto *leave = reinterpret_cast<xcb_leave_notify_event_t *>(event);
        if (leave->event == wid_) {
            if (hover(-1, -1)) {
                repaint();
            }
            return true;
        }
        break;
    }
    }
    return false;
}

void XCBInputWindow::repaint() {
    if (!visible()) {
        return;
    }
    if (auto *surface = prerender()) {
        cairo_t *c = cairo_create(surface);
        paint(c, width(), height(), /*scale=*/1.0);
        cairo_destroy(c);
        render();
    }
}

} // namespace fcitx::classicui
