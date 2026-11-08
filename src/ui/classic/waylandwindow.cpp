/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandwindow.h"
#include <algorithm>
#include <cstdint>
#include <utility>
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
        auto iter =
            std::find(enteredOutputs_.begin(), enteredOutputs_.end(), output);
        if (iter == enteredOutputs_.end()) {
            enteredOutputs_.push_back(output);
        }
        setCurrentOutput(output);
    });
    leaveConn_ = surface_->leave().connect([this](wayland::WlOutput *output) {
        auto iter =
            std::find(enteredOutputs_.begin(), enteredOutputs_.end(), output);
        if (iter != enteredOutputs_.end()) {
            enteredOutputs_.erase(iter);
        }
        if (currentOutput_ == output) {
            setCurrentOutput(enteredOutputs_.empty() ? nullptr
                                                     : enteredOutputs_.back());
        }
    });
}

void WaylandWindow::setCurrentOutput(wayland::WlOutput *output) {
    outputDoneConn_.disconnect();
    currentOutput_ = output;
    if (!output) {
        setOutputSize(0, 0);
        return;
    }

    outputDoneConn_ = output->done().connect(
        [this, output]() { updateOutputInformation(output); });
    updateOutputInformation(output);
}

void WaylandWindow::updateOutputInformation(wayland::WlOutput *output) {
    const auto *info = ui_->display()->outputInformation(output);
    if (!info) {
        setOutputSize(0, 0);
        return;
    }
    int width = info->width();
    int height = info->height();
    if (info->transform() == WL_OUTPUT_TRANSFORM_90 ||
        info->transform() == WL_OUTPUT_TRANSFORM_270 ||
        info->transform() == WL_OUTPUT_TRANSFORM_FLIPPED_90 ||
        info->transform() == WL_OUTPUT_TRANSFORM_FLIPPED_270) {
        std::swap(width, height);
    }
    const int scale = std::max(1, info->scale());
    setOutputSize(width / scale, height / scale);
    if (setScaleAndTransform(scale, info->transform())) {
        scheduleRepaint();
    }
}

void WaylandWindow::setOutputSize(int width, int height) {
    if (outputWidth_ == width && outputHeight_ == height) {
        return;
    }
    outputWidth_ = width;
    outputHeight_ = height;
    outputSizeChanged_();
}

void WaylandWindow::destroyWindow() {
    resetFractionalScale();
    enterConn_.disconnect();
    leaveConn_.disconnect();
    enteredOutputs_.clear();
    setCurrentOutput(nullptr);
    surface_.reset();
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
