//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "waylandinputwindow.h"
#include "waylandui.h"
#include "waylandwindow.h"
#include "zwp_input_panel_v1.h"
#include <linux/input-event-codes.h>

fcitx::classicui::WaylandInputWindow::WaylandInputWindow(WaylandUI *ui)
    : fcitx::classicui::InputWindow(ui->parent()), ui_(ui),
      window_(ui->newWindow()) {
    window_->createWindow();
    window_->repaint().connect([this]() {
        if (auto ic = repaintIC_.get()) {
            if (ic->hasFocus()) {
                update(ic);
            }
        }
    });
    window_->click().connect([this](int x, int y, uint32_t button,
                                    uint32_t state) {
        if (state == WL_POINTER_BUTTON_STATE_PRESSED && button == BTN_LEFT) {
            click(x, y);
        }
    });
    window_->hover().connect([this](int x, int y) {
        if (hover(x, y)) {
            repaint();
        }
    });
    window_->leave().connect([this]() {
        if (hover(-1, -1)) {
            repaint();
        }
    });
    window_->axis().connect([this](int, int, uint32_t axis, wl_fixed_t value) {
        if (axis != WL_POINTER_AXIS_VERTICAL_SCROLL) {
            return;
        }
        scroll_ += value;
        bool triggered = false;
        while (scroll_ >= 2560) {
            scroll_ -= 2560;
            wheel(/*up=*/false);
            triggered = true;
        }
        while (scroll_ <= -2560) {
            scroll_ += 2560;
            wheel(/*up=*/true);
            triggered = true;
        }
        if (triggered) {
            repaint();
        }
    });
    initPanel();
}

void fcitx::classicui::WaylandInputWindow::initPanel() {
    if (panelSurface_) {
        return;
    }
    auto panel = ui_->display()->getGlobals<wayland::ZwpInputPanelV1>();
    if (panel.empty()) {
        return;
    }
    auto iface = panel[0];
    panelSurface_.reset(iface->getInputPanelSurface(window_->surface()));
    panelSurface_->setOverlayPanel();
}

void fcitx::classicui::WaylandInputWindow::resetPanel() {
    panelSurface_.reset();
}

void fcitx::classicui::WaylandInputWindow::update(fcitx::InputContext *ic) {
    InputWindow::update(ic);
    if (!visible()) {
        window_->hide();
        return;
    }
    auto pair = sizeHint();
    int width = pair.first, height = pair.second;

    if (width != window_->width() || height != window_->height()) {
        window_->resize(width, height);
    }

    if (auto surface = window_->prerender()) {
        cairo_t *c = cairo_create(surface);
        paint(c, width, height);
        cairo_destroy(c);
        window_->render();
    } else {
        repaintIC_ = ic->watch();
    }
}

void fcitx::classicui::WaylandInputWindow::repaint() {

    if (auto surface = window_->prerender()) {
        cairo_t *c = cairo_create(surface);
        paint(c, window_->width(), window_->height());
        cairo_destroy(c);
        window_->render();
    }
}
