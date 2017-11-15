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

#include "waylandshmwindow.h"

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
