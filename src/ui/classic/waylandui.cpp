/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandui.h"
#include <algorithm>
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/stringutils.h"
#include "config.h"
#include "display.h"
#include "waylandinputwindow.h"
#include "waylandshmwindow.h"
#include "wl_compositor.h"
#include "wl_seat.h"
#include "wl_shell.h"
#include "wl_shm.h"
#include "zwp_input_panel_v1.h"
#include "zwp_input_popup_surface_v2.h"

namespace fcitx::classicui {

WaylandUI::WaylandUI(ClassicUI *parent, const std::string &name,
                     wl_display *display)
    : parent_(parent), name_(name), display_(static_cast<wayland::Display *>(
                                        wl_display_get_user_data(display))) {
    display_->requestGlobals<wayland::WlCompositor>();
    display_->requestGlobals<wayland::WlShm>();
    display_->requestGlobals<wayland::WlSeat>();
    display_->requestGlobals<wayland::ZwpInputPanelV1>();
    panelConn_ = display_->globalCreated().connect(
        [this](const std::string &name, const std::shared_ptr<void> &) {
            if (name == wayland::ZwpInputPanelV1::interface) {
                if (inputWindow_) {
                    inputWindow_->initPanel();
                }
            } else if (name == wayland::WlCompositor::interface ||
                       name == wayland::WlShm::interface) {
                setupInputWindow();
            } else if (name == wayland::WlSeat::interface) {
                auto seat = display_->getGlobal<wayland::WlSeat>();
                if (seat) {
                    pointer_ = std::make_unique<WaylandPointer>(seat.get());
                }
            }
        });
    panelRemovedConn_ = display_->globalRemoved().connect(
        [this](const std::string &name, const std::shared_ptr<void> &) {
            if (name == wayland::ZwpInputPanelV1::interface) {
                if (inputWindow_) {
                    inputWindow_->resetPanel();
                }
            }
        });
    auto seat = display_->getGlobal<wayland::WlSeat>();
    if (seat) {
        pointer_ = std::make_unique<WaylandPointer>(seat.get());
    }
    display_->sync();
    setupInputWindow();
}

WaylandUI::~WaylandUI() {}

void WaylandUI::update(UserInterfaceComponent component,
                       InputContext *inputContext) {
    if (inputWindow_ && component == UserInterfaceComponent::InputPanel) {
        inputWindow_->update(inputContext);
        display_->flush();
    }
}

void WaylandUI::suspend() { inputWindow_.reset(); }

void WaylandUI::resume() { setupInputWindow(); }

void WaylandUI::setupInputWindow() {
    if (parent_->suspended() || inputWindow_) {
        return;
    }

    // Unable to draw window.
    if (!display_->getGlobal<wayland::WlShm>()) {
        return;
    }
    // Unable to create surface.
    if (!display_->getGlobal<wayland::WlCompositor>()) {
        return;
    }
    inputWindow_ = std::make_unique<WaylandInputWindow>(this);
    inputWindow_->initPanel();
}

std::unique_ptr<WaylandWindow> WaylandUI::newWindow() {
    return std::make_unique<WaylandShmWindow>(this);
}
} // namespace fcitx::classicui
