/*
 * Copyright (C) 2016~2016 by CSSlayer
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

#include "waylandwindow.h"
#include "wl_compositor.h"
#include "wl_output.h"
#include "wl_surface.h"

namespace fcitx {
namespace classicui {

WaylandWindow::WaylandWindow(WaylandUI *ui) : Window(), ui_(ui) {}

WaylandWindow::~WaylandWindow() {}

void WaylandWindow::createWindow() {
    auto compositor = ui_->display()->getGlobal<wayland::WlCompositor>();
    surface_.reset(compositor->createSurface());
    conns_.emplace_back(
        surface_->enter().connect([this](wayland::WlOutput *output) {
            auto info = ui_->display()->outputInformation(output);
            if (!info) {
                return;
            }
            setScale(info->scale());
            setTransform(info->transform());
        }));
}

void WaylandWindow::destroyWindow() { surface_.reset(); }

void bufferToSurfaceSize(enum wl_output_transform buffer_transform,
                         int32_t buffer_scale, int32_t *width,
                         int32_t *height) {
    int32_t tmp;

    switch (buffer_transform) {
    case WL_OUTPUT_TRANSFORM_90:
    case WL_OUTPUT_TRANSFORM_270:
    case WL_OUTPUT_TRANSFORM_FLIPPED_90:
    case WL_OUTPUT_TRANSFORM_FLIPPED_270:
        tmp = *width;
        *width = *height;
        *height = tmp;
        break;
    default:
        break;
    }

    *width /= buffer_scale;
    *height /= buffer_scale;
}

void WaylandWindow::hide() {
    surface_->attach(nullptr, 0, 0);
    surface_->commit();
}
}
}
