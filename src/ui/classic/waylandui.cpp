/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandui.h"
#include <algorithm>
#include "display.h"
#include "org_kde_kwin_blur_manager.h"
#include "waylandcursortheme.h"
#include "waylandinputwindow.h"
#include "waylandshmwindow.h"
#include "wl_compositor.h"
#include "wl_seat.h"
#include "wl_shm.h"
#include "wp_fractional_scale_manager_v1.h"
#include "wp_viewporter.h"
#include "zwp_input_panel_v1.h"

namespace fcitx::classicui {

WaylandUI::WaylandUI(ClassicUI *parent, const std::string &name,
                     wl_display *display)
    : UIInterface("wayland:" + name), parent_(parent),
      display_(
          static_cast<wayland::Display *>(wl_display_get_user_data(display))) {
    display_->requestGlobals<wayland::WlCompositor>();
    display_->requestGlobals<wayland::WlShm>();
    display_->requestGlobals<wayland::WlSeat>();
    display_->requestGlobals<wayland::ZwpInputPanelV1>();
    display_->requestGlobals<wayland::OrgKdeKwinBlurManager>();
    display_->requestGlobals<wayland::WpFractionalScaleManagerV1>();
    display_->requestGlobals<wayland::WpViewporter>();
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
                    pointer_ =
                        std::make_unique<WaylandPointer>(this, seat.get());
                }
            } else if (name == wayland::OrgKdeKwinBlurManager::interface) {
                if (inputWindow_) {
                    inputWindow_->setBlurManager(
                        display_->getGlobal<wayland::OrgKdeKwinBlurManager>());
                }
            } else if (name == wayland::WpFractionalScaleManagerV1::interface ||
                       name == wayland::WpViewporter::interface) {
                if (inputWindow_) {
                    inputWindow_->updateScale();
                }
            }
        });
    panelRemovedConn_ = display_->globalRemoved().connect(
        [this](const std::string &name, const std::shared_ptr<void> &) {
            if (name == wayland::ZwpInputPanelV1::interface) {
                if (inputWindow_) {
                    inputWindow_->resetPanel();
                }
            } else if (name == wayland::OrgKdeKwinBlurManager::interface) {
                if (inputWindow_) {
                    inputWindow_->setBlurManager(nullptr);
                }
            } else if (name == wayland::WpFractionalScaleManagerV1::interface ||
                       name == wayland::WpViewporter::interface) {
                if (inputWindow_) {
                    inputWindow_->updateScale();
                }
            }
        });
    auto seat = display_->getGlobal<wayland::WlSeat>();
    if (seat) {
        pointer_ = std::make_unique<WaylandPointer>(this, seat.get());
    }
    setupInputWindow();
}

WaylandUI::~WaylandUI() {}

void WaylandUI::update(UserInterfaceComponent component,
                       InputContext *inputContext) {
    if (inputWindow_ && component == UserInterfaceComponent::InputPanel) {
        CLASSICUI_DEBUG() << "Update Wayland Input Window";
        inputWindow_->update(inputContext);
    }
}

void WaylandUI::suspend() {
    if (!inputWindow_) {
        return;
    }
    inputWindow_->update(nullptr);
}

void WaylandUI::resume() {
    CLASSICUI_DEBUG() << "Resume WaylandUI display name:" << display_;
    CLASSICUI_DEBUG() << "Wayland Input window is initialized:"
                      << !!inputWindow_;
}

void WaylandUI::setupInputWindow() {
    if (inputWindow_) {
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

    cursorTheme_ = std::make_unique<WaylandCursorTheme>(this);
    inputWindow_ = std::make_unique<WaylandInputWindow>(this);
    inputWindow_->initPanel();
    inputWindow_->setBlurManager(
        display_->getGlobal<wayland::OrgKdeKwinBlurManager>());
}

std::unique_ptr<WaylandWindow> WaylandUI::newWindow() {
    return std::make_unique<WaylandShmWindow>(this);
}
} // namespace fcitx::classicui
