/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandwindow.h"
#include <cstdint>
#include "fcitx-utils/eventloopinterface.h"
#include "waylandui.h"
#include "wl_compositor.h"
#include "wl_output.h"
#include "wl_surface.h"
#include "wp_fractional_scale_manager_v1.h"
#include "wp_viewporter.h"

namespace fcitx::classicui {

WaylandWindow::WaylandWindow(WaylandUI *ui) : ui_(ui) {}

WaylandWindow::~WaylandWindow() {}

void WaylandWindow::createWindow() {
    auto compositor = ui_->display()->getGlobal<wayland::WlCompositor>();
    if (!compositor) {
        return;
    }
    surface_.reset(compositor->createSurface());
    surface_->setUserData(this);
    updateScale();

    enterConn_ = surface_->enter().connect([this](wayland::WlOutput *output) {
        const auto *info = ui_->display()->outputInformation(output);
        if (!info) {
            return;
        }
        if (setScaleAndTransform(info->scale(), info->transform())) {
            scheduleRepaint();
        }
    });
}

void WaylandWindow::destroyWindow() {
    resetFractionalScale();
    surface_.reset();
    enterConn_.disconnect();
}

void WaylandWindow::resetFractionalScale() {
    viewport_.reset();
    fractionalScale_.reset();
    viewporter_.reset();
    fractionalScaleManager_.reset();
}

void WaylandWindow::scheduleRepaint() {
    repaintEvent_ = ui_->parent()->instance()->eventLoop().addDeferEvent(
        [this](EventSource *) {
            repaint_();
            repaintEvent_.reset();
            return true;
        });
}

void WaylandWindow::updateScale() {
    if (!surface_) {
        return;
    }

    if (!*ui_->parent()->config().fractionalScale) {
        resetFractionalScale();
        return;
    }

    viewporter_ = ui_->display()->getGlobal<wayland::WpViewporter>();
    fractionalScaleManager_ =
        ui_->display()->getGlobal<wayland::WpFractionalScaleManagerV1>();
    if (viewporter_ && fractionalScaleManager_) {
        viewport_.reset(viewporter_->getViewport(surface_.get()));
        fractionalScale_.reset(
            fractionalScaleManager_->getFractionalScale(surface_.get()));
    }

    if (viewport_ && fractionalScale_) {
        fractionalScale_->preferredScale().connect([this](uint32_t scale) {
            if (scale != lastFractionalScale_) {
                lastFractionalScale_ = scale;
                scheduleRepaint();
            }
        });
    } else {
        resetFractionalScale();
    }
}

} // namespace fcitx::classicui
