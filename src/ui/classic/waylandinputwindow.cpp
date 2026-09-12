/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandinputwindow.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cairo.h>
#include <pango/pango-fontmap.h>
#include <pango/pangocairo.h>
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/rect.h"
#include "fcitx/inputcontext.h"
#include "common.h"
#include "ext_background_effect_manager_v1.h"
#include "inputwindow.h"
#include "theme.h"
#include "waylandim_public.h"
#include "waylandui.h"
#include "waylandwindow.h"
#include "wl_compositor.h"
#include "wl_region.h"
#include "zwp_input_method_v2.h"
#include "zwp_input_panel_v1.h"

#ifdef __linux__
#include <linux/input-event-codes.h>
#elif __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#else
#define BTN_LEFT 0x110
#define BTN_RIGHT 0x111
#endif

namespace fcitx::classicui {

/** Initializes the Wayland input window and its event handlers. */
WaylandInputWindow::WaylandInputWindow(WaylandUI *ui)
    : InputWindow(ui->parent()), ui_(ui), window_(ui->newWindow()) {
    menuContext_.reset(pango_font_map_create_context(fontMap_.get()));
    menuLayout_.reset(pango_layout_new(menuContext_.get()));
    window_->createWindow();
    window_->repaint().connect([this]() {
        if (auto *ic = repaintIC_.get()) {
            if (ic->hasFocus()) {
                if (candidateMenuVisible_) {
                    repaint();
                } else {
                    update(ic);
                }
            }
        }
    });
    window_->click().connect(
        [this](int x, int y, uint32_t button, uint32_t state) {
            if (state != WL_POINTER_BUTTON_STATE_PRESSED) {
                return;
            }
            if (button == BTN_RIGHT) {
                showCandidateMenu(x, y);
            } else if (button == BTN_LEFT) {
                if (candidateMenuVisible_) {
                    clickCandidateMenu(x, y);
                } else {
                    click(x, y);
                }
            }
        });
    window_->hover().connect([this](int x, int y) {
        bool needRepaint = false;
        if (candidateMenuVisible_) {
            needRepaint = hoverCandidateMenu(x, y);
            if (!candidateMenuRegion_.contains(x, y)) {
                needRepaint = hover(x, y) || needRepaint;
            }
        } else {
            needRepaint = hover(x, y);
        }
        if (needRepaint) {
            repaint();
        }
    });
    window_->leave().connect([this]() {
        bool needRepaint = hover(-1, -1);
        if (candidateMenuHoveredIndex_ != -1) {
            candidateMenuHoveredIndex_ = -1;
            needRepaint = true;
        }
        if (needRepaint) {
            repaint();
        }
    });
    window_->touchDown().connect([this](int x, int y) {
        if (candidateMenuVisible_) {
            clickCandidateMenu(x, y);
        } else {
            click(x, y);
        }
    });
    window_->touchUp().connect([](int, int) {
        // do nothing
    });
    window_->axis().connect([this](int, int, uint32_t axis, wl_fixed_t value) {
        if (axis != WL_POINTER_AXIS_VERTICAL_SCROLL) {
            return;
        }
        if (candidateMenuVisible_) {
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

/** Hides the candidate menu and discards its temporary state. */
void WaylandInputWindow::clearCandidateMenu() {
    candidateMenuVisible_ = false;
    candidateMenuCandidate_ = nullptr;
    candidateMenuList_.reset();
    candidateMenuActions_.clear();
    candidateMenuRegions_.clear();
    candidateMenuRegion_ = Rect();
    candidateMenuX_ = candidateMenuY_ = 0;
    candidateMenuWidth_ = candidateMenuHeight_ = 0;
    candidateMenuItemWidth_ = candidateMenuItemHeight_ = 0;
    candidateMenuHasCheckable_ = false;
    candidateMenuHoveredIndex_ = -1;
}

/** Restores the input panel surface after a candidate menu closes. */
void WaylandInputWindow::restorePanelSize() {
    if (!window_ || panelWidth_ <= 0 || panelHeight_ <= 0 ||
        (window_->width() == panelWidth_ &&
         window_->height() == panelHeight_)) {
        return;
    }
    window_->resize(panelWidth_, panelHeight_);
    updateBlur();
}

/** Builds and shows the candidate action menu at the pointer position. */
void WaylandInputWindow::showCandidateMenu(int x, int y) {
    auto dismiss = [this]() {
        if (!candidateMenuVisible_) {
            return;
        }
        clearCandidateMenu();
        restorePanelSize();
        repaint();
    };

    auto *inputContext = inputContext_.get();
    if (!inputContext) {
        dismiss();
        return;
    }

    const auto candidateList = inputContext->inputPanel().candidateList();
    if (!candidateList) {
        dismiss();
        return;
    }

    const CandidateWord *candidate = nullptr;
    for (size_t idx = 0, e = candidateRegions_.size(); idx < e; idx++) {
        if (candidateRegions_[idx].contains(x, y)) {
            candidate = nthCandidateIgnorePlaceholder(*candidateList, idx);
            break;
        }
    }

    auto *actionable = candidateList->toActionable();
    if (!candidate || !actionable || !actionable->hasAction(*candidate)) {
        dismiss();
        return;
    }

    auto actions = actionable->candidateActions(*candidate);
    if (actions.empty() ||
        std::ranges::all_of(actions, &CandidateAction::isSeparator) ||
        !menuContext_ || !menuLayout_) {
        dismiss();
        return;
    }

    clearCandidateMenu();
    candidateMenuList_ = candidateList;
    candidateMenuCandidate_ = candidate;
    candidateMenuActions_ = std::move(actions);

    auto &theme = ui_->parent()->theme();
    const auto &menu = *theme.menu;
    const auto &contentMargin = *menu.contentMargin;
    const auto &textMargin = *menu.textMargin;
    const auto &checkBox = theme.loadBackground(*menu.checkBox);
    const auto &separator = theme.loadBackground(*menu.separator);

    auto *fontDescription = pango_font_description_from_string(
        ui_->parent()->config().menuFont->c_str());
    pango_context_set_font_description(menuContext_.get(), fontDescription);
    pango_layout_set_font_description(menuLayout_.get(), fontDescription);
    pango_font_description_free(fontDescription);

    int maxTextWidth = 0;
    int maxTextHeight = 0;
    for (const auto &action : candidateMenuActions_) {
        candidateMenuHasCheckable_ =
            candidateMenuHasCheckable_ ||
            (action.isCheckable() && !action.isSeparator());
        if (action.isSeparator()) {
            continue;
        }
        pango_layout_set_text(menuLayout_.get(), action.text().c_str(),
                              action.text().size());
        int textWidth = 0;
        int textHeight = 0;
        pango_layout_get_pixel_size(menuLayout_.get(), &textWidth, &textHeight);
        maxTextWidth = std::max(maxTextWidth, textWidth);
        maxTextHeight = std::max(maxTextHeight, textHeight);
    }

    int maxItemWidth = maxTextWidth;
    int maxItemHeight = maxTextHeight;
    if (candidateMenuHasCheckable_) {
        maxItemWidth += checkBox.width();
        maxItemHeight = std::max(maxItemHeight, checkBox.height());
    }
    candidateMenuItemWidth_ =
        maxItemWidth + *textMargin.marginLeft + *textMargin.marginRight;
    candidateMenuItemHeight_ =
        maxItemHeight + *textMargin.marginTop + *textMargin.marginBottom;

    candidateMenuWidth_ = *contentMargin.marginLeft + candidateMenuItemWidth_ +
                          *contentMargin.marginRight;
    candidateMenuHeight_ =
        *contentMargin.marginTop + *contentMargin.marginBottom;
    for (const auto &action : candidateMenuActions_) {
        candidateMenuHeight_ +=
            action.isSeparator()
                ? (separator.isPattern() ? 2 : separator.height())
                : candidateMenuItemHeight_;
    }
    if (candidateMenuActions_.size() > 1) {
        candidateMenuHeight_ +=
            (candidateMenuActions_.size() - 1) * *menu.spacing;
    }
    candidateMenuWidth_ = std::max(candidateMenuWidth_, 1);
    candidateMenuHeight_ = std::max(candidateMenuHeight_, 1);

    const int panelWidth = panelWidth_ > 0 ? panelWidth_ : window_->width();
    const int panelHeight = panelHeight_ > 0 ? panelHeight_ : window_->height();
    candidateMenuX_ =
        std::min(std::max(x, 0), std::max(0, panelWidth - candidateMenuWidth_));
    candidateMenuY_ = std::min(std::max(y, 0),
                               std::max(0, panelHeight - candidateMenuHeight_));
    candidateMenuRegion_.setPosition(candidateMenuX_, candidateMenuY_)
        .setSize(candidateMenuWidth_, candidateMenuHeight_);

    candidateMenuRegions_.reserve(candidateMenuActions_.size());
    int itemY = candidateMenuY_ + *contentMargin.marginTop;
    for (size_t i = 0; i < candidateMenuActions_.size(); i++) {
        const auto &action = candidateMenuActions_[i];
        const int itemHeight =
            action.isSeparator()
                ? (separator.isPattern() ? 2 : separator.height())
                : candidateMenuItemHeight_;
        const int itemWidth = action.isSeparator()
                                  ? candidateMenuWidth_ -
                                        *contentMargin.marginLeft -
                                        *contentMargin.marginRight
                                  : candidateMenuItemWidth_;
        candidateMenuRegions_.push_back(
            Rect()
                .setPosition(candidateMenuX_ + *contentMargin.marginLeft, itemY)
                .setSize(std::max(itemWidth, 0), std::max(itemHeight, 0)));
        itemY += itemHeight;
        if (i + 1 < candidateMenuActions_.size()) {
            itemY += *menu.spacing;
        }
    }

    candidateMenuVisible_ = true;
    candidateMenuHoveredIndex_ = -1;
    const int width =
        std::max(panelWidth, candidateMenuX_ + candidateMenuWidth_);
    const int height =
        std::max(panelHeight, candidateMenuY_ + candidateMenuHeight_);
    if (window_->width() != width || window_->height() != height) {
        window_->resize(width, height);
        updateBlur();
    }
    repaint();
}

/** Updates the candidate menu item under the pointer. */
bool WaylandInputWindow::hoverCandidateMenu(int x, int y) {
    if (!candidateMenuVisible_) {
        return false;
    }

    int index = -1;
    for (size_t i = 0; i < candidateMenuActions_.size(); i++) {
        if (!candidateMenuActions_[i].isSeparator() &&
            candidateMenuRegions_[i].contains(x, y)) {
            index = static_cast<int>(i);
            break;
        }
    }
    if (candidateMenuHoveredIndex_ == index) {
        return false;
    }
    candidateMenuHoveredIndex_ = index;
    return true;
}

/** Activates or dismisses the candidate menu after a left click. */
void WaylandInputWindow::clickCandidateMenu(int x, int y) {
    if (!candidateMenuVisible_) {
        return;
    }

    int index = -1;
    for (size_t i = 0; i < candidateMenuActions_.size(); i++) {
        if (!candidateMenuActions_[i].isSeparator() &&
            candidateMenuRegions_[i].contains(x, y)) {
            index = static_cast<int>(i);
            break;
        }
    }
    if (index < 0) {
        clearCandidateMenu();
        restorePanelSize();
        repaint();
        return;
    }

    const auto candidateList = candidateMenuList_;
    const auto *candidate = candidateMenuCandidate_;
    const int id = candidateMenuActions_[index].id();
    clearCandidateMenu();
    restorePanelSize();
    if (candidateList && candidate) {
        if (auto *actionable = candidateList->toActionable()) {
            actionable->triggerAction(*candidate, id);
        }
    }
    repaint();
}

/** Paints the candidate action menu over the input panel. */
void WaylandInputWindow::paintCandidateMenu(cairo_t *cr) {
    if (!candidateMenuVisible_) {
        return;
    }

    auto &theme = ui_->parent()->theme();
    const auto &menu = *theme.menu;
    const auto &textMargin = *menu.textMargin;
    const auto &checkBox = theme.loadBackground(*menu.checkBox);
    theme.paint(cr, *menu.background, candidateMenuX_, candidateMenuY_,
                candidateMenuWidth_, candidateMenuHeight_, 1.0);

    for (size_t i = 0; i < candidateMenuActions_.size(); i++) {
        const auto &action = candidateMenuActions_[i];
        const auto &region = candidateMenuRegions_[i];
        if (action.isSeparator()) {
            theme.paint(cr, *menu.separator, region.left(), region.top(),
                        region.width(), region.height(), 1.0);
            continue;
        }

        if (candidateMenuHoveredIndex_ == static_cast<int>(i)) {
            theme.paint(cr, *menu.highlight, region.left(), region.top(),
                        region.width(), region.height(), 1.0);
        }

        if (action.isChecked()) {
            const int checkX = region.left() + *textMargin.marginLeft;
            const int checkY =
                region.top() + (region.height() - checkBox.height()) / 2;
            theme.paint(cr, *menu.checkBox, checkX, checkY, -1, -1, 1.0);
        }

        pango_layout_set_text(menuLayout_.get(), action.text().c_str(),
                              action.text().size());
        int textHeight = 0;
        pango_layout_get_pixel_size(menuLayout_.get(), nullptr, &textHeight);
        const int textX = region.left() + *textMargin.marginLeft +
                          (candidateMenuHasCheckable_ ? checkBox.width() : 0);
        const int textY = region.top() + (region.height() - textHeight) / 2;

        cairo_save(cr);
        cairoSetSourceColor(cr,
                            candidateMenuHoveredIndex_ == static_cast<int>(i)
                                ? theme.menuSelectedItemText()
                                : theme.menuText());
        cairo_move_to(cr, textX, textY);
        pango_cairo_show_layout(cr, menuLayout_.get());
        cairo_restore(cr);
    }
}

/** Creates the Wayland input panel surface when necessary. */
void WaylandInputWindow::initPanel() {
    if (!window_->surface()) {
        window_->createWindow();
        updateBlur();
    }

    setFontDPI(*parent_->config().forceWaylandDPI);
}

/** Sets the compositor background-effect manager for the input panel. */
void WaylandInputWindow::setBlurManager(
    std::shared_ptr<wayland::ExtBackgroundEffectManagerV1> blur) {
    blurManager_ = std::move(blur);
    updateBlur();
}

/** Updates the compositor blur region for the current panel size. */
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
    auto width = window_->width();
    auto height = window_->height();
    Rect rect(0, 0, width, height);
    shrink(rect, *ui_->parent()->theme().inputPanel->blurMargin);
    if (!*ui_->parent()->theme().inputPanel->enableBlur || rect.isEmpty()) {
        return;
    }
    std::vector<uint32_t> data;
    std::unique_ptr<wayland::WlRegion> region(compositor->createRegion());
    if (ui_->parent()->theme().inputPanel->blurMask->empty()) {
        region->add(rect.left(), rect.top(), rect.width(), rect.height());
    } else {
        auto regions =
            parent_->theme().mask(parent_->theme().maskConfig(), width, height);
        for (const auto &rect : regions) {
            region->add(rect.left(), rect.top(), rect.width(), rect.height());
        }
    }
    blur_.reset(blurManager_->getBackgroundEffect(window_->surface()));
    blur_->setBlurRegion(region.get());
}

/** Updates the input window buffer scale. */
void WaylandInputWindow::updateScale() { window_->updateScale(); }

/** Releases the current Wayland input panel surface. */
void WaylandInputWindow::resetPanel() { panelSurface_.reset(); }

/** Updates the input panel contents, surface, and candidate menu state. */
void WaylandInputWindow::update(fcitx::InputContext *ic) {
    clearCandidateMenu();
    const auto oldVisible = visible();
    auto [width, height] = InputWindow::update(ic);
    CLASSICUI_DEBUG() << "Wayland Input Window visible:" << visible()
                      << " for IC program:"
                      << (ic ? ic->program() : std::string("-")) << " frontend:"
                      << (ic ? ic->frontendName() : std::string("-"));
    if (!oldVisible && !visible()) {
        CLASSICUI_DEBUG() << "Wayland Input Window has been hidden.";
        return;
    }

    if (!visible()) {
        CLASSICUI_DEBUG() << "Hide Wayland Input Window.";
        hoverIndex_ = -1;
        window_->hide();
        repaintIC_.unwatch();
        panelSurface_.reset();
        panelSurfaceV2_.reset();
        blur_.reset();
        window_->destroyWindow();
        return;
    }

    assert(!visible() || ic != nullptr);

    CLASSICUI_DEBUG()
        << "Wayland Input Window is visible, ensure surface is created.";
    initPanel();
    if (ic->frontendName() == "wayland_v2") {
        if (!panelSurfaceV2_ || ic != v2IC_.get()) {
            auto *waylandim = ui_->parent()->waylandim();
            if (!waylandim) {
                CLASSICUI_ERROR()
                    << "Failed to request waylandim addon, this should not "
                       "happen since we have wayland_v2 input context.";
                return;
            }
            auto *im = waylandim->call<IWaylandIMModule::getInputMethodV2>(ic);
            if (!im) {
                CLASSICUI_ERROR()
                    << "Failed to request get zwp_input_method_v2 object, this "
                       "should not happen since we have wayland_v2 input "
                       "context.";
                return;
            }
            v2IC_ = ic->watch();
            panelSurfaceV2_.reset();
            panelSurfaceV2_.reset(im->getInputPopupSurface(window_->surface()));
        }
    } else if (ic->frontendName() == "wayland") {
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
        CLASSICUI_DEBUG() << "No Panel surface available, return.";
        return;
    }

    if (width != window_->width() || height != window_->height()) {
        window_->resize(width, height);
        updateBlur();
    }
    panelWidth_ = width;
    panelHeight_ = height;

    if (auto *surface = window_->prerender()) {
        cairo_t *c = cairo_create(surface);
        cairo_surface_set_device_scale(
            cairo_get_target(c),
            window_->bufferScale() / WaylandWindow::ScaleDominatorF,
            window_->bufferScale() / WaylandWindow::ScaleDominatorF);
        paint(c, width, height);
        paintCandidateMenu(c);
        cairo_destroy(c);
        window_->render();
    }
    repaintIC_ = ic->watch();
}

/** Repaints the visible Wayland input window. */
void WaylandInputWindow::repaint() {
    if (!visible()) {
        return;
    }

    if (auto *surface = window_->prerender()) {
        cairo_t *c = cairo_create(surface);
        cairo_surface_set_device_scale(
            cairo_get_target(c),
            window_->bufferScale() / WaylandWindow::ScaleDominatorF,
            window_->bufferScale() / WaylandWindow::ScaleDominatorF);
        paint(c, window_->width(), window_->height());
        paintCandidateMenu(c);
        cairo_destroy(c);
        window_->render();
    }
}

} // namespace fcitx::classicui
