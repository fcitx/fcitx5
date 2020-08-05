/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "waylandwindow.h"
#include "wl_compositor.h"
#include "wl_output.h"
#include "wl_surface.h"

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
    conns_.emplace_back(
        surface_->enter().connect([this](wayland::WlOutput *output) {
            const auto *info = ui_->display()->outputInformation(output);
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
} // namespace fcitx::classicui
