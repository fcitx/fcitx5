/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "waylandcandidatemenu.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <cairo.h>
#include <pango/pangocairo.h>
#include "fcitx/inputcontext.h"
#include "candidatemenuplacement.h"
#include "common.h"
#include "theme.h"
#include "waylandui.h"
#include "waylandwindow.h"
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
    : ui_(ui), parentWindow_(parentWindow), window_(ui->newWindow()) {
    fontMap_.reset(pango_cairo_font_map_new());
    fontMapDefaultDPI_ = pango_cairo_font_map_get_resolution(
        PANGO_CAIRO_FONT_MAP(fontMap_.get()));
    context_.reset(pango_font_map_create_context(fontMap_.get()));
    layout_.reset(pango_layout_new(context_.get()));
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
        if (visible_) {
            clear();
        }
    });
    window_->touchDown().connect([this](int x, int y) { click(x, y); });
    window_->touchUp().connect([](int, int) {});
}

void WaylandCandidateMenu::setFontDPI(int dpi) {
    pango_cairo_font_map_set_resolution(PANGO_CAIRO_FONT_MAP(fontMap_.get()),
                                        dpi <= 0 ? fontMapDefaultDPI_ : dpi);
    pango_cairo_context_set_resolution(context_.get(), dpi);
}

void WaylandCandidateMenu::createWindow() {
    if (!window_->surface()) {
        window_->createWindow();
    }
}

void WaylandCandidateMenu::destroyWindow() {
    clear();
    subsurface_.reset();
    window_->destroyWindow();
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
    subsurface_.reset(subcompositor->getSubsurface(
        window_->surface(), parentWindow_->surface()));
    if (!subsurface_) {
        return false;
    }
    subsurface_->setDesync();
    return true;
}

/** Hides the candidate menu and discards its temporary state. */
void WaylandCandidateMenu::clear() {
    if (visible_ && window_) {
        window_->hide();
    }
    visible_ = false;
    candidate_ = nullptr;
    candidateList_.reset();
    actions_.clear();
    regions_.clear();
    width_ = height_ = 0;
    itemWidth_ = itemHeight_ = 0;
    hasCheckable_ = false;
    hoveredIndex_ = -1;
}

/** Builds and shows the candidate action menu at the pointer position. */
void WaylandCandidateMenu::show(
    InputContext *inputContext, const std::vector<Rect> &candidateRegions,
    int x, int y, const std::optional<Rect> &textInputRectangle) {
    if (!inputContext) {
        clear();
        return;
    }

    const auto candidateList = inputContext->inputPanel().candidateList();
    if (!candidateList) {
        clear();
        return;
    }

    const CandidateWord *candidate = nullptr;
    Rect candidateRegion;
    for (size_t idx = 0, e = candidateRegions.size(); idx < e; idx++) {
        if (candidateRegions[idx].contains(x, y)) {
            candidate = nthCandidateIgnorePlaceholder(*candidateList, idx);
            candidateRegion = candidateRegions[idx];
            break;
        }
    }

    auto *actionable = candidateList->toActionable();
    if (!candidate || !actionable || !actionable->hasAction(*candidate)) {
        clear();
        return;
    }

    auto actions = actionable->candidateActions(*candidate);
    if (actions.empty() ||
        std::ranges::all_of(actions, &CandidateAction::isSeparator) ||
        !context_ || !layout_) {
        clear();
        return;
    }

    if (!createSubsurface()) {
        clear();
        return;
    }

    clear();
    candidateList_ = candidateList;
    candidate_ = candidate;
    actions_ = std::move(actions);

    auto &theme = ui_->parent()->theme();
    const auto &menu = *theme.menu;
    const auto &contentMargin = *menu.contentMargin;
    const auto &textMargin = *menu.textMargin;
    const auto &checkBox = theme.loadBackground(*menu.checkBox);
    const auto &separator = theme.loadBackground(*menu.separator);

    auto *fontDescription = pango_font_description_from_string(
        ui_->parent()->config().menuFont->c_str());
    pango_context_set_font_description(context_.get(), fontDescription);
    pango_layout_set_font_description(layout_.get(), fontDescription);
    pango_font_description_free(fontDescription);

    int maxTextWidth = 0;
    int maxTextHeight = 0;
    for (const auto &action : actions_) {
        hasCheckable_ = hasCheckable_ ||
                        (action.isCheckable() && !action.isSeparator());
        if (action.isSeparator()) {
            continue;
        }
        pango_layout_set_text(layout_.get(), action.text().c_str(),
                              action.text().size());
        int textWidth = 0;
        int textHeight = 0;
        pango_layout_get_pixel_size(layout_.get(), &textWidth, &textHeight);
        maxTextWidth = std::max(maxTextWidth, textWidth);
        maxTextHeight = std::max(maxTextHeight, textHeight);
    }

    int maxItemWidth = maxTextWidth;
    int maxItemHeight = maxTextHeight;
    if (hasCheckable_) {
        maxItemWidth += checkBox.width();
        maxItemHeight = std::max(maxItemHeight, checkBox.height());
    }
    itemWidth_ = maxItemWidth + *textMargin.marginLeft + *textMargin.marginRight;
    itemHeight_ =
        maxItemHeight + *textMargin.marginTop + *textMargin.marginBottom;

    width_ = *contentMargin.marginLeft + itemWidth_ +
             *contentMargin.marginRight;
    height_ = *contentMargin.marginTop + *contentMargin.marginBottom;
    for (const auto &action : actions_) {
        height_ += action.isSeparator()
                       ? (separator.isPattern() ? 2 : separator.height())
                       : itemHeight_;
    }
    if (actions_.size() > 1) {
        height_ += (actions_.size() - 1) * *menu.spacing;
    }
    width_ = std::max(width_, 1);
    height_ = std::max(height_, 1);

    anchor_ = candidateRegion;

    regions_.reserve(actions_.size());
    int itemY = *contentMargin.marginTop;
    for (size_t i = 0; i < actions_.size(); i++) {
        const auto &action = actions_[i];
        const int itemHeight =
            action.isSeparator()
                ? (separator.isPattern() ? 2 : separator.height())
                : itemHeight_;
        const int itemWidth = action.isSeparator()
                                  ? width_ - *contentMargin.marginLeft -
                                        *contentMargin.marginRight
                                  : itemWidth_;
        regions_.push_back(
            Rect()
                .setPosition(*contentMargin.marginLeft, itemY)
                .setSize(std::max(itemWidth, 0), std::max(itemHeight, 0)));
        itemY += itemHeight;
        if (i + 1 < actions_.size()) {
            itemY += *menu.spacing;
        }
    }
    visible_ = true;
    hoveredIndex_ = -1;
    window_->resize(width_, height_);
    repaint();
    position(textInputRectangle);
}

void WaylandCandidateMenu::position(
    const std::optional<Rect> &textInputRectangle) {
    if (!visible_ || !subsurface_ || !parentWindow_->surface()) {
        return;
    }
    const auto position = candidateMenuPosition(
        anchor_, parentWindow_->width(), parentWindow_->height(), width_,
        height_, textInputRectangle);
    subsurface_->setPosition(position.left(), position.top());
    // Subsurface position changes are double-buffered on the parent.
    parentWindow_->surface()->commit();
}

/** Updates the candidate menu item under the pointer. */
bool WaylandCandidateMenu::hover(int x, int y) {
    if (!visible_) {
        return false;
    }

    int index = -1;
    for (size_t i = 0; i < actions_.size(); i++) {
        if (!actions_[i].isSeparator() && regions_[i].contains(x, y)) {
            index = static_cast<int>(i);
            break;
        }
    }
    if (hoveredIndex_ == index) {
        return false;
    }
    hoveredIndex_ = index;
    return true;
}

/** Activates or dismisses the candidate menu after a left click. */
void WaylandCandidateMenu::click(int x, int y) {
    if (!visible_) {
        return;
    }

    int index = -1;
    for (size_t i = 0; i < actions_.size(); i++) {
        if (!actions_[i].isSeparator() && regions_[i].contains(x, y)) {
            index = static_cast<int>(i);
            break;
        }
    }
    if (index < 0) {
        clear();
        return;
    }

    const auto candidateList = candidateList_;
    const auto *candidate = candidate_;
    const int id = actions_[index].id();
    clear();
    if (candidateList && candidate) {
        if (auto *actionable = candidateList->toActionable()) {
            actionable->triggerAction(*candidate, id);
        }
    }
}

/** Paints the candidate action menu on its independent popup surface. */
void WaylandCandidateMenu::paint(cairo_t *cr) {
    if (!visible_) {
        return;
    }

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    auto &theme = ui_->parent()->theme();
    const auto &menu = *theme.menu;
    const auto &textMargin = *menu.textMargin;
    const auto &checkBox = theme.loadBackground(*menu.checkBox);
    theme.paint(cr, *menu.background, 0, 0, width_, height_, 1.0);

    for (size_t i = 0; i < actions_.size(); i++) {
        const auto &action = actions_[i];
        const auto &region = regions_[i];
        if (action.isSeparator()) {
            theme.paint(cr, *menu.separator, region.left(), region.top(),
                        region.width(), region.height(), 1.0);
            continue;
        }

        if (hoveredIndex_ == static_cast<int>(i)) {
            theme.paint(cr, *menu.highlight, region.left(), region.top(),
                        region.width(), region.height(), 1.0);
        }

        if (action.isChecked()) {
            const int checkX = region.left() + *textMargin.marginLeft;
            const int checkY =
                region.top() + (region.height() - checkBox.height()) / 2;
            theme.paint(cr, *menu.checkBox, checkX, checkY, -1, -1, 1.0);
        }

        pango_layout_set_text(layout_.get(), action.text().c_str(),
                              action.text().size());
        int textHeight = 0;
        pango_layout_get_pixel_size(layout_.get(), nullptr, &textHeight);
        const int textX = region.left() + *textMargin.marginLeft +
                          (hasCheckable_ ? checkBox.width() : 0);
        const int textY = region.top() + (region.height() - textHeight) / 2;

        cairo_save(cr);
        cairoSetSourceColor(cr,
                            hoveredIndex_ == static_cast<int>(i)
                                ? theme.menuSelectedItemText()
                                : theme.menuText());
        cairo_move_to(cr, textX, textY);
        pango_cairo_show_layout(cr, layout_.get());
        cairo_restore(cr);
    }
}

void WaylandCandidateMenu::repaint() {
    if (!visible_ || !window_) {
        return;
    }
    if (auto *surface = window_->prerender()) {
        cairo_t *c = cairo_create(surface);
        cairo_surface_set_device_scale(cairo_get_target(c),
                                       window_->bufferScale() /
                                           WaylandWindow::ScaleDominatorF,
                                       window_->bufferScale() /
                                           WaylandWindow::ScaleDominatorF);
        paint(c);
        cairo_destroy(c);
        window_->render();
    }
}

} // namespace fcitx::classicui
