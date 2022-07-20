/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandinputwindow.h"
#include "waylandim_public.h"
#include "waylandui.h"
#include "waylandwindow.h"
#include "wl_compositor.h"
#include "wl_region.h"
#include "zwp_input_panel_v1.h"
#include "zwp_input_popup_surface_v2.h"

#ifdef __linux__
#include <linux/input-event-codes.h>
#elif __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#else
#define BTN_LEFT 0x110
#endif

namespace fcitx::classicui {

WaylandInputWindow::WaylandInputWindow(WaylandUI *ui)
    : InputWindow(ui->parent()), ui_(ui), window_(ui->newWindow()) {
    window_->createWindow();
    window_->repaint().connect([this]() {
        if (auto *ic = repaintIC_.get()) {
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
    window_->touchDown().connect([this](int x, int y) { click(x, y); });
    window_->touchUp().connect([](int, int) {
        // do nothing
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

void WaylandInputWindow::initPanel() {
    if (!window_->surface()) {
        window_->createWindow();
        updateBlur();
    }

    setFontDPI(*parent_->config().forceWaylandDPI);
}

void WaylandInputWindow::setBlurManager(
    std::shared_ptr<wayland::OrgKdeKwinBlurManager> blur) {
    blurManager_ = blur;
    updateBlur();
}

void WaylandInputWindow::updateBlur() {
    if (!window_->surface()) {
        return;
    }
    blur_.reset();
    if (!blurManager_) {
        return;
    }

    auto compositor = ui_->display()->getGlobal<wayland::WlCompositor>();
    if (!compositor) {
        return;
    }
    auto width = window_->width(), height = window_->height();
    Rect rect(0, 0, width, height);
    shrink(rect, *ui_->parent()->theme().inputPanel->blurMargin);
    if (!*ui_->parent()->theme().inputPanel->enableBlur || rect.isEmpty()) {
        return;
    } else {
        std::vector<uint32_t> data;
        std::unique_ptr<wayland::WlRegion> region(compositor->createRegion());
        if (ui_->parent()->theme().inputPanel->blurMask->empty()) {
            region->add(rect.left(), rect.top(), rect.width(), rect.height());
        } else {
            auto regions = parent_->theme().mask(parent_->theme().maskConfig(),
                                                 width, height);
            for (const auto &rect : regions) {
                region->add(rect.left(), rect.top(), rect.width(),
                            rect.height());
            }
        }
        blur_.reset(blurManager_->create(window_->surface()));
        blur_->setRegion(region.get());
        blur_->commit();
    }
}

void WaylandInputWindow::resetPanel() { panelSurface_.reset(); }

void WaylandInputWindow::update(fcitx::InputContext *ic) {
    const auto oldVisible = visible();
    auto [width, height] = InputWindow::update(ic);
    if (!oldVisible && !visible()) {
        return;
    }

    if (!visible()) {
        window_->hide();
        panelSurface_.reset();
        panelSurfaceV2_.reset();
        blur_.reset();
        window_->destroyWindow();
        return;
    }

    assert(!visible() || ic != nullptr);

    initPanel();
    if (ic->frontend() == std::string_view("wayland_v2")) {
        if (!panelSurfaceV2_ || ic != v2IC_.get()) {
            v2IC_ = ic->watch();
            auto *im = ui_->parent()
                           ->waylandim()
                           ->call<IWaylandIMModule::getInputMethodV2>(ic);
            panelSurfaceV2_.reset();
            panelSurfaceV2_.reset(im->getInputPopupSurface(window_->surface()));
        }
    } else if (ic->frontend() == std::string_view("wayland")) {
        auto panel = ui_->display()->getGlobal<wayland::ZwpInputPanelV1>();
        if (!panel) {
            return;
        }
        if (!panelSurface_) {
            panelSurface_.reset(
                panel->getInputPanelSurface(window_->surface()));
            panelSurface_->setOverlayPanel();
        }
    }
    if (!panelSurface_ && !panelSurfaceV2_) {
        return;
    }

    if (width != window_->width() || height != window_->height()) {
        window_->resize(width, height);
        updateBlur();
    }

    if (auto *surface = window_->prerender()) {
        cairo_t *c = cairo_create(surface);
        cairo_scale(c, window_->scale(), window_->scale());
        paint(c, width, height);
        cairo_destroy(c);
        window_->render();
    } else {
        repaintIC_ = ic->watch();
    }
}

void WaylandInputWindow::repaint() {
    if (!visible()) {
        return;
    }

    if (auto *surface = window_->prerender()) {
        cairo_t *c = cairo_create(surface);
        cairo_scale(c, window_->scale(), window_->scale());
        paint(c, window_->width(), window_->height());
        cairo_destroy(c);
        window_->render();
    }
}

} // namespace fcitx::classicui
