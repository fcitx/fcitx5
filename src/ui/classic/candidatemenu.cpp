/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "candidatemenu.h"
#include <algorithm>
#include <utility>
#include <pango/pangocairo.h>
#include "fcitx/inputcontext.h"
#include "classicui.h"
#include "theme.h"

namespace fcitx::classicui {

CandidateMenu::CandidateMenu(ClassicUI *parent) : parent_(parent) {
    fontMap_.reset(pango_cairo_font_map_new());
    fontMapDefaultDPI_ = pango_cairo_font_map_get_resolution(
        PANGO_CAIRO_FONT_MAP(fontMap_.get()));
    context_.reset(pango_font_map_create_context(fontMap_.get()));
    layout_.reset(pango_layout_new(context_.get()));
}

void CandidateMenu::setFontDPI(int dpi) {
    pango_cairo_font_map_set_resolution(PANGO_CAIRO_FONT_MAP(fontMap_.get()),
                                        dpi <= 0 ? fontMapDefaultDPI_ : dpi);
    pango_cairo_context_set_resolution(context_.get(), dpi);
}

void CandidateMenu::clear() {
    visible_ = false;
    inputContext_ = {};
    candidateList_.reset();
    candidate_ = nullptr;
    actions_.clear();
    hoveredIndex_ = -1;
}

bool CandidateMenu::show(InputContext *inputContext,
                         const std::vector<Rect> &candidateRegions, int x,
                         int y) {
    clear();
    if (!inputContext || !context_ || !layout_) {
        return false;
    }
    auto candidateList = inputContext->inputPanel().candidateList();
    if (!candidateList) {
        return false;
    }

    const CandidateWord *candidate = nullptr;
    Rect anchor;
    size_t candidateIndex = 0;
    for (size_t idx = 0; idx < candidateRegions.size(); idx++) {
        if (candidateRegions[idx].contains(x, y)) {
            candidate = nthCandidateIgnorePlaceholder(*candidateList, idx);
            anchor = candidateRegions[idx];
            candidateIndex = idx;
            break;
        }
    }
    auto *actionable = candidateList->toActionable();
    if (!candidate || !actionable || !actionable->hasAction(*candidate)) {
        return false;
    }
    auto actions = actionable->candidateActions(*candidate);
    if (actions.empty() ||
        std::ranges::all_of(actions, &CandidateAction::isSeparator)) {
        return false;
    }

    auto &theme = parent_->theme();
    const auto &menu = *theme.menu;
    const auto &content = *menu.contentMargin;
    const auto &text = *menu.textMargin;
    const auto &highlight = *menu.highlight->margin;
    const auto &checkBox = theme.loadBackground(*menu.checkBox);
    const auto &separator = theme.loadBackground(*menu.separator);

    auto *fontDescription =
        pango_font_description_from_string(parent_->config().menuFont->c_str());
    pango_context_set_font_description(context_.get(), fontDescription);
    pango_layout_set_font_description(layout_.get(), fontDescription);
    pango_font_description_free(fontDescription);
    pango_layout_context_changed(layout_.get());

    CandidateMenuLayout::Style style;
    style.content = {*content.marginLeft, *content.marginRight,
                     *content.marginTop, *content.marginBottom};
    style.text = {*text.marginLeft, *text.marginRight, *text.marginTop,
                  *text.marginBottom};
    style.highlight = {*highlight.marginLeft, *highlight.marginRight,
                       *highlight.marginTop, *highlight.marginBottom};
    style.spacing = *menu.spacing;
    style.separatorHeight = separator.isPattern() ? 2 : separator.height();
    if (std::ranges::any_of(actions, [](const auto &action) {
            return action.isCheckable() && !action.isSeparator();
        })) {
        style.checkWidth = checkBox.width();
        style.checkHeight = checkBox.height();
    }

    std::vector<CandidateMenuLayout::Item> items;
    items.reserve(actions.size());
    for (const auto &action : actions) {
        int textWidth = 0;
        int textHeight = 0;
        if (!action.isSeparator()) {
            pango_layout_set_text(layout_.get(), action.text().c_str(),
                                  action.text().size());
            pango_layout_get_pixel_size(layout_.get(), &textWidth, &textHeight);
        }
        items.push_back({textWidth, textHeight, action.isSeparator()});
    }
    layoutGeometry_.update(style, items);
    inputContext_ = inputContext->watch();
    candidateList_ = std::move(candidateList);
    candidate_ = candidate;
    candidateIndex_ = candidateIndex;
    anchor_ = anchor;
    actions_ = std::move(actions);
    visible_ = true;
    return true;
}

Rect CandidateMenu::highlightRegion(const Rect &region) const {
    const auto &margin = *parent_->theme().menu->highlight->margin;
    return Rect()
        .setPosition(region.left() - *margin.marginLeft,
                     region.top() - *margin.marginTop)
        .setSize(region.width() + *margin.marginLeft + *margin.marginRight,
                 region.height() + *margin.marginTop + *margin.marginBottom);
}

bool CandidateMenu::hover(int x, int y) {
    if (!visible_) {
        return false;
    }
    const int index = layoutGeometry_.indexAt(x, y);
    if (hoveredIndex_ == index) {
        return false;
    }
    hoveredIndex_ = index;
    return true;
}

std::optional<CandidateMenu::Selection>
CandidateMenu::selectionAt(int x, int y) const {
    if (!visible_) {
        return std::nullopt;
    }
    const int index = layoutGeometry_.indexAt(x, y);
    if (index < 0 || actions_[index].isSeparator()) {
        return std::nullopt;
    }
    return Selection{inputContext_, candidateList_, candidate_, candidateIndex_,
                     actions_[index].id()};
}

void CandidateMenu::Selection::activate() const {
    auto *context = inputContext.get();
    if (!context || context->inputPanel().candidateList() != candidateList ||
        nthCandidateIgnorePlaceholder(*candidateList, candidateIndex) !=
            candidate) {
        return;
    }
    if (auto *actionable = candidateList->toActionable()) {
        actionable->triggerAction(*candidate, id);
    }
}

void CandidateMenu::paint(cairo_t *cr) {
    if (!visible_) {
        return;
    }
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    auto &theme = parent_->theme();
    const auto &menu = *theme.menu;
    theme.paint(cr, *menu.background, 0, 0, width(), height(), 1.0);
    for (size_t i = 0; i < actions_.size(); i++) {
        const auto &action = actions_[i];
        const auto &row = layoutGeometry_.row(i);
        if (action.isSeparator()) {
            theme.paint(cr, *menu.separator, row.region.left(),
                        row.region.top(), row.region.width(),
                        row.region.height(), 1.0);
            continue;
        }
        if (hoveredIndex_ == static_cast<int>(i)) {
            const auto highlight = highlightRegion(row.region);
            theme.paint(cr, *menu.highlight, highlight.left(), highlight.top(),
                        highlight.width(), highlight.height(), 1.0);
        }
        if (action.isChecked()) {
            theme.paint(cr, *menu.checkBox, row.check.left(), row.check.top(),
                        -1, -1, 1.0);
        }
        pango_layout_set_text(layout_.get(), action.text().c_str(),
                              action.text().size());
        cairo_save(cr);
        cairoSetSourceColor(cr, hoveredIndex_ == static_cast<int>(i)
                                    ? theme.menuSelectedItemText()
                                    : theme.menuText());
        cairo_move_to(cr, row.text.left(), row.text.top());
        pango_cairo_show_layout(cr, layout_.get());
        cairo_restore(cr);
    }
}

} // namespace fcitx::classicui
