/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandshmwindow.h"
#include "common.h"

fcitx::classicui::WaylandShmWindow::WaylandShmWindow(
    fcitx::classicui::WaylandUI *ui)
    : fcitx::classicui::WaylandWindow(ui),
      shm_(ui->display()->getGlobal<wayland::WlShm>()) {}

fcitx::classicui::WaylandShmWindow::~WaylandShmWindow() {}

void fcitx::classicui::WaylandShmWindow::destroyWindow() {
    buffers_.clear();
    buffer_ = nullptr;
    WaylandWindow::destroyWindow();
}

void fcitx::classicui::WaylandShmWindow::newBuffer() {
    if (!shm_) {
        return;
    }
    buffers_.emplace_back(std::make_unique<wayland::Buffer>(
        shm_.get(), width_, height_, WL_SHM_FORMAT_ARGB8888));
    buffers_.back()->rendered().connect([this]() {
        if (pending_) {
            pending_ = false;

            CLASSICUI_DEBUG() << "Trigger repaint";
            repaint_();
        }
    });
}

cairo_surface_t *fcitx::classicui::WaylandShmWindow::prerender() {
    // We use double buffer.
    decltype(buffers_)::iterator iter;
    for (iter = buffers_.begin(); iter != buffers_.end(); iter++) {
        CLASSICUI_DEBUG() << "Buffer state: " << (*iter).get() << " "
                          << (*iter)->busy();
        if (!(*iter)->busy()) {
            break;
        }
    }

    if (iter != buffers_.end() &&
        ((*iter)->width() != width_ || (*iter)->height() != height_)) {
        buffers_.erase(iter);
        iter = buffers_.end();
    }

    if (iter == buffers_.end() && buffers_.size() < 2) {
        newBuffer();
        if (buffers_.size()) {
            iter = std::prev(buffers_.end());
        }
    }

    if (iter == buffers_.end()) {
        CLASSICUI_DEBUG() << "Couldn't find avail buffer.";
        pending_ = true;
        // All buffers are busy.
        buffer_ = nullptr;
        return nullptr;
    } else {
        pending_ = false;
        buffer_ = iter->get();
    }

    auto cairoSurface = buffer_->cairoSurface();
    if (!cairoSurface) {
        buffer_ = nullptr;
        return nullptr;
    }
    return cairoSurface;
}

void fcitx::classicui::WaylandShmWindow::render() {
    if (!buffer_) {
        return;
    }

    surface_->setBufferScale(1);
    buffer_->attachToSurface(surface_.get());
}

void fcitx::classicui::WaylandShmWindow::hide() {
    surface_->attach(nullptr, 0, 0);
    surface_->commit();
}
