/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "xcbcandidatemenu.h"
#include <unistd.h>
#include <algorithm>
#include <climits>
#include <cstdint>
#include <vector>
#include <cairo.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>
#include "fcitx-utils/rect.h"
#include "fcitx/inputcontext.h"
#include "classicui.h"
#include "theme.h"
#include "xcb_public.h"
#include "xcbui.h"

namespace fcitx::classicui {

XCBCandidateMenu::XCBCandidateMenu(XCBUI *ui)
    : XCBWindow(ui), menu_(ui->parent()),
      atomBlur_(ui->parent()->xcb()->call<IXCBModule::atom>(
          ui->displayName(), "_KDE_NET_WM_BLUR_BEHIND_REGION", false)) {}

bool XCBCandidateMenu::show(InputContext *inputContext,
                            const std::vector<Rect> &regions, int x, int y,
                            int rootX, int rootY) {
    if (activationTimer_) {
        return false;
    }
    hide();
    ui_->fontOption().setupPangoContext(menu_.fontContext());
    if (!menu_.show(inputContext, regions, x, y)) {
        return false;
    }
    if (wid_ && vid_ != ui_->visualId()) {
        destroyWindow();
    }
    if (!wid_) {
        createWindow(ui_->visualId());
    }
    setScale(scaleForDPI(ui_->dpiByPosition(rootX, rootY)));
    resize(menu_.width(), menu_.height());
    updateBlur();
    repaint();

    const Rect *closestScreen = nullptr;
    int shortestDistance = INT_MAX;
    for (const auto &[screen, unused] : ui_->screenRects()) {
        const int distance = screen.distance(rootX, rootY);
        if (distance < shortestDistance) {
            shortestDistance = distance;
            closestScreen = &screen;
        }
    }
    int menuX = rootX + 1;
    int menuY = rootY;
    if (closestScreen) {
        if (menuX + physicalWidth_ > closestScreen->right()) {
            menuX = rootX - physicalWidth_;
        }
        if (menuY + physicalHeight_ > closestScreen->bottom()) {
            menuY = rootY - physicalHeight_;
        }
        menuY = std::max(menuY, closestScreen->top());
    }
    xcb_params_configure_window_t config;
    config.x = menuX;
    config.y = menuY;
    config.stack_mode = XCB_STACK_MODE_ABOVE;
    xcb_aux_configure_window(ui_->connection(), wid_,
                             XCB_CONFIG_WINDOW_STACK_MODE |
                                 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             &config);
    visible_ = true;
    xcb_map_window(ui_->connection(), wid_);
    ui_->grabPointer(this);
    return true;
}

void XCBCandidateMenu::hide() {
    if (visible_) {
        visible_ = false;
        xcb_unmap_window(ui_->connection(), wid_);
        if (ui_->pointerGrabber() == this) {
            ui_->ungrabPointer();
        }
    }
    menu_.clear();
}

bool XCBCandidateMenu::filterEvent(xcb_generic_event_t *event) {
    switch (event->response_type & ~0x80) {
    case XCB_EXPOSE: {
        auto *expose = reinterpret_cast<xcb_expose_event_t *>(event);
        if (expose->window == wid_) {
            repaint();
            return true;
        }
        break;
    }
    case XCB_FOCUS_OUT: {
        auto *focus = reinterpret_cast<xcb_focus_out_event_t *>(event);
        if (focus->event == wid_ &&
            focus->detail != XCB_NOTIFY_DETAIL_POINTER) {
            hide();
            return true;
        }
        break;
    }
    case XCB_BUTTON_PRESS: {
        auto *button = reinterpret_cast<xcb_button_press_event_t *>(event);
        if (button->event != wid_) {
            break;
        }
        auto selection =
            button->detail == XCB_BUTTON_INDEX_1
                ? menu_.selectionAt(logicalFromPhysical(button->event_x),
                                    logicalFromPhysical(button->event_y))
                : std::nullopt;
        hide();
        if (selection) {
            activationTimer_ =
                ui_->parent()->instance()->eventLoop().addTimeEvent(
                    CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 30000, 0,
                    [that = watch(), selection = *selection](EventSourceTime *,
                                                             uint64_t) {
                        if (auto *window = that.get()) {
                            selection.activate();
                            window->activationTimer_.reset();
                        }
                        return true;
                    });
        }
        return true;
    }
    case XCB_MOTION_NOTIFY: {
        auto *motion = reinterpret_cast<xcb_motion_notify_event_t *>(event);
        if (motion->event == wid_ && visible_) {
            if (menu_.hover(logicalFromPhysical(motion->event_x),
                            logicalFromPhysical(motion->event_y))) {
                repaint();
            }
            return true;
        }
        break;
    }
    case XCB_LEAVE_NOTIFY: {
        auto *leave = reinterpret_cast<xcb_leave_notify_event_t *>(event);
        if (leave->event == wid_ && visible_) {
            if (menu_.hover(-1, -1)) {
                repaint();
            }
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

void XCBCandidateMenu::postCreateWindow() {
    if (ui_->ewmh()->_NET_WM_WINDOW_TYPE_MENU &&
        ui_->ewmh()->_NET_WM_WINDOW_TYPE_POPUP_MENU &&
        ui_->ewmh()->_NET_WM_WINDOW_TYPE) {
        uint32_t types[] = {ui_->ewmh()->_NET_WM_WINDOW_TYPE_MENU,
                            ui_->ewmh()->_NET_WM_WINDOW_TYPE_POPUP_MENU};
        xcb_ewmh_set_wm_window_type(ui_->ewmh(), wid_, 2, types);
    }
    if (ui_->ewmh()->_NET_WM_PID) {
        xcb_ewmh_set_wm_pid(ui_->ewmh(), wid_, getpid());
    }
    const char name[] = "Fcitx5 Candidate Menu";
    xcb_icccm_set_wm_name(ui_->connection(), wid_, XCB_ATOM_STRING, 8,
                          sizeof(name) - 1, name);
    const char klass[] = "fcitx\0fcitx";
    xcb_icccm_set_wm_class(ui_->connection(), wid_, sizeof(klass) - 1, klass);
    addEventMaskToWindow(ui_->connection(), wid_,
                         XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
                             XCB_EVENT_MASK_FOCUS_CHANGE |
                             XCB_EVENT_MASK_POINTER_MOTION |
                             XCB_EVENT_MASK_LEAVE_WINDOW);
}

void XCBCandidateMenu::repaint() {
    if (!menu_.visible()) {
        return;
    }
    if (auto *surface = prerender()) {
        auto *cr = cairo_create(surface);
        menu_.paint(cr);
        cairo_destroy(cr);
        render();
    }
}

void XCBCandidateMenu::updateBlur() {
    if (!atomBlur_) {
        return;
    }
    auto &theme = ui_->parent()->theme();
    const auto &config = *theme.menu;
    Rect rect(0, 0, menu_.width(), menu_.height());
    shrink(rect, *config.blurMargin);
    if (!*config.enableBlur || rect.isEmpty()) {
        xcb_delete_property(ui_->connection(), wid_, atomBlur_);
        return;
    }
    std::vector<uint32_t> data;
    if (config.blurMask->empty()) {
        auto physical = physicalFromLogical(rect);
        data = {static_cast<uint32_t>(physical.left()),
                static_cast<uint32_t>(physical.top()),
                static_cast<uint32_t>(physical.width()),
                static_cast<uint32_t>(physical.height())};
    } else {
        for (const auto &mask : theme.mask(theme.menuBlurMaskConfig(),
                                           menu_.width(), menu_.height())) {
            auto physical = physicalFromLogical(mask);
            data.push_back(physical.left());
            data.push_back(physical.top());
            data.push_back(physical.width());
            data.push_back(physical.height());
        }
    }
    xcb_change_property(ui_->connection(), XCB_PROP_MODE_REPLACE, wid_,
                        atomBlur_, XCB_ATOM_CARDINAL, 32, data.size(),
                        data.data());
}

} // namespace fcitx::classicui
