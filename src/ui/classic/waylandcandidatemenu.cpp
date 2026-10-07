/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "waylandcandidatemenu.h"
#include <cstdint>
#include <memory>
#include <utility>
#include <cairo.h>
#include "candidatemenuplacement.h"
#include "ext_background_effect_manager_v1.h"
#include "theme.h"
#include "waylandui.h"
#include "waylandwindow.h"
#include "wl_compositor.h"
#include "wl_region.h"
#include "wl_subcompositor.h"

#ifdef __linux__
#include <linux/input-event-codes.h>
#elif __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#else
#define BTN_LEFT 0x110
#define BTN_RIGHT 0x111
#endif

namespace fcitx::classicui {

WaylandCandidateMenu::WaylandCandidateMenu(WaylandUI *ui,
                                           WaylandWindow *parentWindow)
    : ui_(ui), parentWindow_(parentWindow), window_(ui->newWindow()),
      menu_(ui->parent()) {
    createWindow();
    window_->repaint().connect([this]() { repaint(); });
    window_->click().connect(
        [this](int x, int y, uint32_t button, uint32_t state) {
            if (state != WL_POINTER_BUTTON_STATE_PRESSED) {
                return;
            }
            if (button == BTN_LEFT) {
                click(x, y);
            } else if (button == BTN_RIGHT) {
                clear();
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
    window_->touchUp().connect([](int, int) {});
}

void WaylandCandidateMenu::setFontDPI(int dpi) { menu_.setFontDPI(dpi); }

void WaylandCandidateMenu::createWindow() {
    if (!window_->surface()) {
        window_->createWindow();
        updateBlur();
    }
}

void WaylandCandidateMenu::destroyWindow() {
    clear();
    blur_.reset();
    subsurface_.reset();
    window_->destroyWindow();
}

void WaylandCandidateMenu::setBlurManager(
    std::shared_ptr<wayland::ExtBackgroundEffectManagerV1> blur) {
    blurManager_ = std::move(blur);
    updateBlur();
}

void WaylandCandidateMenu::updateBlur() {
    blur_.reset();
    if (!blurManager_ || !window_->surface()) {
        return;
    }

    auto &theme = ui_->parent()->theme();
    const auto &menu = *theme.menu;
    Rect rect(0, 0, window_->width(), window_->height());
    shrink(rect, *menu.blurMargin);
    if (!*menu.enableBlur || rect.isEmpty()) {
        return;
    }

    auto compositor = ui_->display()->getGlobal<wayland::WlCompositor>();
    if (!compositor) {
        return;
    }
    std::unique_ptr<wayland::WlRegion> region(compositor->createRegion());
    if (menu.blurMask->empty()) {
        region->add(rect.left(), rect.top(), rect.width(), rect.height());
    } else {
        for (const auto &maskRect :
             theme.mask(theme.menuBlurMaskConfig(), window_->width(),
                        window_->height())) {
            region->add(maskRect.left(), maskRect.top(), maskRect.width(),
                        maskRect.height());
        }
    }
    blur_.reset(blurManager_->getBackgroundEffect(window_->surface()));
    blur_->setBlurRegion(region.get());
}

void WaylandCandidateMenu::resetSubsurface() {
    clear();
    subsurface_.reset();
}

void WaylandCandidateMenu::updateScale() { window_->updateScale(); }

bool WaylandCandidateMenu::createSubsurface() {
    if (subsurface_) {
        return true;
    }
    auto subcompositor = ui_->display()->getGlobal<wayland::WlSubcompositor>();
    if (!subcompositor || !window_->surface() || !parentWindow_->surface()) {
        return false;
    }
    subsurface_.reset(subcompositor->getSubsurface(window_->surface(),
                                                   parentWindow_->surface()));
    if (!subsurface_) {
        return false;
    }
    subsurface_->setDesync();
    return true;
}

void WaylandCandidateMenu::clear() {
    if (visible_ && window_) {
        window_->hide();
    }
    visible_ = false;
    menu_.clear();
}

void WaylandCandidateMenu::show(InputContext *inputContext,
                                const std::vector<Rect> &candidateRegions,
                                int x, int y,
                                const std::optional<Rect> &textInputRectangle) {
    clear();
    if (!menu_.show(inputContext, candidateRegions, x, y)) {
        return;
    }
    if (!createSubsurface()) {
        clear();
        return;
    }
    visible_ = true;
    window_->resize(menu_.width(), menu_.height());
    updateBlur();
    repaint();
    position(textInputRectangle);
}

void WaylandCandidateMenu::position(
    const std::optional<Rect> &textInputRectangle) {
    if (!visible_ || !subsurface_ || !parentWindow_->surface()) {
        return;
    }
    const auto position = candidateMenuPosition(
        menu_.anchor(), parentWindow_->width(), parentWindow_->height(),
        menu_.width(), menu_.height(), textInputRectangle);
    subsurface_->setPosition(position.left(), position.top());
    // Subsurface position changes are double-buffered on the parent.
    parentWindow_->surface()->commit();
}

bool WaylandCandidateMenu::hover(int x, int y) {
    return visible_ && menu_.hover(x, y);
}

void WaylandCandidateMenu::click(int x, int y) {
    if (!visible_) {
        return;
    }
    auto selection = menu_.selectionAt(x, y);
    clear();
    if (selection) {
        selection->activate();
    }
}

void WaylandCandidateMenu::repaint() {
    if (!visible_ || !window_) {
        return;
    }
    if (auto *surface = window_->prerender()) {
        cairo_t *c = cairo_create(surface);
        cairo_surface_set_device_scale(
            cairo_get_target(c),
            window_->bufferScale() / WaylandWindow::ScaleDominatorF,
            window_->bufferScale() / WaylandWindow::ScaleDominatorF);
        menu_.paint(c);
        cairo_destroy(c);
        window_->render();
    }
}

} // namespace fcitx::classicui
