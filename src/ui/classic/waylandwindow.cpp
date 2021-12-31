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
            if (setScaleAndTransform(info->scale(), info->transform())) {
                repaint_();
            }
        }));
}

void WaylandWindow::destroyWindow() { surface_.reset(); }

} // namespace fcitx::classicui
