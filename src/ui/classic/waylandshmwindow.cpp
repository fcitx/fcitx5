/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandshmwindow.h"
#include <cstdint>
#include <iterator>
#include <memory>
#include <cairo.h>
#include "fcitx-utils/eventloopinterface.h"
#include "buffer.h"
#include "common.h"
#include "waylandui.h"
#include "waylandwindow.h"
#include "wl_shm.h"

namespace fcitx::classicui {

namespace {

void surfaceToBufferSize(unsigned int buffer_scale, uint32_t *width,
                         uint32_t *height) {
    // round (size * scale), but all integer calculation.
    *width = (*width * buffer_scale + (WaylandWindow::ScaleDominator / 2)) /
             WaylandWindow::ScaleDominator;
    *height = (*height * buffer_scale + (WaylandWindow::ScaleDominator / 2)) /
              WaylandWindow::ScaleDominator;
}

} // namespace

WaylandShmWindow::WaylandShmWindow(WaylandUI *ui)
    : WaylandWindow(ui), shm_(ui->display()->getGlobal<wayland::WlShm>()) {}

WaylandShmWindow::~WaylandShmWindow() {}

void WaylandShmWindow::destroyWindow() {
    buffers_.clear();
    buffer_ = nullptr;
    WaylandWindow::destroyWindow();
}

void WaylandShmWindow::newBuffer(uint32_t width, uint32_t height) {
    if (!shm_) {
        return;
    }
    buffers_.emplace_back(std::make_unique<wayland::Buffer>(
        shm_.get(), width, height, WL_SHM_FORMAT_ARGB8888));
    buffers_.back()->rendered().connect([this]() {
        // Use defer event here, otherwise repaint may delete buffer and cause
        // problem.
        deferEvent_ = ui_->parent()->instance()->eventLoop().addDeferEvent(
            [this](EventSource *) {
                if (pending_) {
                    pending_ = false;

                    CLASSICUI_DEBUG() << "Trigger repaint";
                    repaint_();
                }
                deferEvent_.reset();
                return true;
            });
    });
}

cairo_surface_t *WaylandShmWindow::prerender() {
    // We use double buffer.
    decltype(buffers_)::iterator iter;
    for (iter = buffers_.begin(); iter != buffers_.end(); iter++) {
        CLASSICUI_DEBUG() << "Buffer state: " << (*iter).get() << " "
                          << (*iter)->busy();
        if (!(*iter)->busy()) {
            break;
        }
    }

    uint32_t bufferWidth = width_;
    uint32_t bufferHeight = height_;
    surfaceToBufferSize(bufferScale(), &bufferWidth, &bufferHeight);

    if (iter != buffers_.end() && ((*iter)->width() != bufferWidth ||
                                   (*iter)->height() != bufferHeight)) {
        buffers_.erase(iter);
        iter = buffers_.end();
    }

    if (iter == buffers_.end() && buffers_.size() < 2) {
        newBuffer(bufferWidth, bufferHeight);
        if (!buffers_.empty()) {
            iter = std::prev(buffers_.end());
        }
    }

    if (iter == buffers_.end()) {
        CLASSICUI_DEBUG() << "Couldn't find avail buffer.";
        pending_ = true;
        // All buffers are busy.
        buffer_ = nullptr;
        return nullptr;
    }
    pending_ = false;
    buffer_ = iter->get();

    auto *cairoSurface = buffer_->cairoSurface();
    if (!cairoSurface) {
        buffer_ = nullptr;
        return nullptr;
    }
    return cairoSurface;
}

void WaylandShmWindow::render() {
    if (!buffer_) {
        return;
    }

    if (viewport_) {
        if (buffer_->attachToSurface(surface_.get(), 1)) {
            viewport_->setDestination(width_, height_);
            surface_->commit();
        }
    } else {
        if (buffer_->attachToSurface(surface_.get(), lastOutputScale_)) {
            surface_->commit();
        }
    }
}

void WaylandShmWindow::hide() {
    if (!surface_) {
        return;
    }
    surface_->attach(nullptr, 0, 0);
    surface_->commit();
}

} // namespace fcitx::classicui
