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
#include "inputwindow.h"
#include "classicui.h"
#include "fcitx-utils/color.h"
#include "fcitx-utils/log.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include <functional>
#include <initializer_list>
#include <limits>
#include <pango/pangocairo.h>

namespace fcitx {

namespace classicui {

InputWindow::InputWindow(ClassicUI *parent)
    : parent_(parent), context_(nullptr, &g_object_unref),
      upperLayout_(nullptr, &g_object_unref),
      lowerLayout_(nullptr, &g_object_unref) {
    auto fontMap = pango_cairo_font_map_get_default();
    context_.reset(pango_font_map_create_context(fontMap));
    upperLayout_.reset(pango_layout_new(context_.get()));
    lowerLayout_.reset(pango_layout_new(context_.get()));
}

void InputWindow::insertAttr(PangoAttrList *attrList, TextFormatFlags format,
                             int start, int end, bool highlight) const {
    if (format & TextFormatFlag::Underline) {
        auto attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);
    }
    if (format & TextFormatFlag::Italic) {
        auto attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);
    }
    if (format & TextFormatFlag::Strike) {
        auto attr = pango_attr_strikethrough_new(true);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);
    }
    if (format & TextFormatFlag::Bold) {
        auto attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);
    }
    Color color =
        (format & TextFormatFlag::HighLight)
            ? *parent_->theme().inputPanel->highlightColor
            : (highlight ? *parent_->theme().inputPanel->highlightCandidateColor
                         : *parent_->theme().inputPanel->normalColor);
    auto scale = std::numeric_limits<unsigned short>::max();
    auto attr = pango_attr_foreground_new(
        color.redF() * scale, color.greenF() * scale, color.blueF() * scale);
    attr->start_index = start;
    attr->end_index = end;
    pango_attr_list_insert(attrList, attr);

    if (format & TextFormatFlag::HighLight) {
        auto background =
            *parent_->theme().inputPanel->highlightBackgroundColor;
        attr = pango_attr_background_new(background.redF() * scale,
                                         background.greenF() * scale,
                                         background.blueF() * scale);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);
    }
}

void InputWindow::appendText(std::string &s, PangoAttrList *attrList,
                             PangoAttrList *highlightAttrList,
                             const Text &text) {
    for (size_t i = 0, e = text.size(); i < e; i++) {
        auto start = s.size();
        s.append(text.stringAt(i));
        auto end = s.size();
        if (start == end) {
            continue;
        }
        const auto format = text.formatAt(i);
        insertAttr(attrList, format, start, end, false);
        if (highlightAttrList) {
            insertAttr(highlightAttrList, format, start, end, true);
        }
    }
}

void InputWindow::resizeCandidates(size_t n) {
    while (labelLayouts_.size() < n) {
        labelLayouts_.emplace_back(pango_layout_new(context_.get()),
                                   &g_object_unref);
    }
    while (candidateLayouts_.size() < n) {
        candidateLayouts_.emplace_back(pango_layout_new(context_.get()),
                                       &g_object_unref);
    }
    for (auto attrLists :
         {&labelAttrLists_, &candidateAttrLists_, &highlightLabelAttrLists_,
          &highlightCandidateAttrLists_}) {
        while (attrLists->size() < n) {
            attrLists->emplace_back(pango_attr_list_new(),
                                    &pango_attr_list_unref);
        }
    }

    nCandidates_ = n;
}

void InputWindow::setTextToLayout(
    PangoLayout *layout, PangoAttrList *attrList,
    PangoAttrList *highlightAttrList,

    std::initializer_list<std::reference_wrapper<const Text>> texts) {
    if (attrList) {
        pango_attr_list_ref(attrList);
    } else {
        attrList = pango_attr_list_new();
    }
    std::string line;
    for (auto &text : texts) {
        appendText(line, attrList, highlightAttrList, text);
    }

    pango_layout_set_text(layout, line.c_str(), line.size());
    pango_layout_set_attributes(layout, attrList);
    pango_layout_set_height(layout, -(2 << 20));

    pango_attr_list_unref(attrList);
}

void InputWindow::update(InputContext *inputContext) {
    if (parent_->suspended()) {
        visible_ = false;
        return;
    }
    // | aux up | preedit
    // | aux down
    // | 1 candidate | 2 ...
    // or
    // | aux up | preedit
    // | aux down
    // | candidate 1
    // | candidate 2
    // | candidate 3
    auto instance = parent_->instance();
    auto &inputPanel = inputContext->inputPanel();
    inputContext_ = inputContext->watch();

    cursor_ = -1;
    auto preedit = instance->outputFilter(inputContext, inputPanel.preedit());
    auto auxUp = instance->outputFilter(inputContext, inputPanel.auxUp());
    pango_layout_set_single_paragraph_mode(upperLayout_.get(), true);
    setTextToLayout(upperLayout_.get(), nullptr, nullptr, {auxUp, preedit});
    if (preedit.cursor() >= 0 &&
        static_cast<size_t>(preedit.cursor()) <= preedit.textLength()) {
        cursor_ = preedit.cursor() + auxUp.toString().size();
    }

    auto auxDown = instance->outputFilter(inputContext, inputPanel.auxDown());
    setTextToLayout(lowerLayout_.get(), nullptr, nullptr, {auxDown});

    if (auto candidateList = inputPanel.candidateList()) {
        // Count non-placeholder candidates.
        int count = 0;

        for (int i = 0, e = candidateList->size(); i < e; i++) {
            auto candidate = candidateList->candidate(i);
            if (candidate->isPlaceHolder()) {
                continue;
            }
            count++;
        }
        resizeCandidates(count);

        int localIndex = 0;
        for (int i = 0, e = candidateList->size(); i < e; i++) {
            auto candidate = candidateList->candidate(i);
            // Skip placeholder.
            if (candidate->isPlaceHolder()) {
                continue;
            }

            Text labelText = candidate->hasCustomLabel()
                                 ? candidate->customLabel()
                                 : candidateList->label(i);

            labelText = instance->outputFilter(inputContext, labelText);
            setTextToLayout(labelLayouts_[localIndex].get(),
                            labelAttrLists_[localIndex].get(),
                            highlightLabelAttrLists_[localIndex].get(),
                            {labelText});
            auto candidateText =
                instance->outputFilter(inputContext, candidate->text());
            setTextToLayout(candidateLayouts_[localIndex].get(),
                            candidateAttrLists_[localIndex].get(),
                            highlightCandidateAttrLists_[localIndex].get(),
                            {candidateText});
            localIndex++;
        }

        layoutHint_ = candidateList->layoutHint();
        candidateIndex_ = candidateList->cursorIndex();
    } else {
        nCandidates_ = 0;
        candidateIndex_ = -1;
    }

    visible_ = nCandidates_ ||
               pango_layout_get_character_count(upperLayout_.get()) ||
               pango_layout_get_character_count(lowerLayout_.get());
}

std::pair<unsigned int, unsigned int> InputWindow::sizeHint() {
    auto fontDesc =
        pango_font_description_from_string(parent_->config().font->c_str());
    pango_context_set_font_description(context_.get(), fontDesc);
    pango_cairo_context_set_resolution(context_.get(), dpi_);
    pango_font_description_free(fontDesc);
    pango_layout_context_changed(upperLayout_.get());
    pango_layout_context_changed(lowerLayout_.get());
    for (size_t i = 0; i < nCandidates_; i++) {
        pango_layout_context_changed(labelLayouts_[i].get());
        pango_layout_context_changed(candidateLayouts_[i].get());
    }
    auto metrics = pango_context_get_metrics(
        context_.get(), pango_context_get_font_description(context_.get()),
        pango_context_get_language(context_.get()));
    auto minH = pango_font_metrics_get_ascent(metrics) +
                pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);

    size_t width = 0;
    size_t height = 0;
    auto updateIfLarger = [](size_t &m, size_t n) {
        if (n > m) {
            m = n;
        }
    };
    int w, h;

    const auto &textMargin = *parent_->theme().inputPanel->textMargin;
    auto extraW = *textMargin.marginLeft + *textMargin.marginRight;
    auto extraH = *textMargin.marginTop + *textMargin.marginBottom;
    if (pango_layout_get_character_count(upperLayout_.get())) {
        pango_layout_get_pixel_size(upperLayout_.get(), &w, &h);
        h = std::max(PANGO_PIXELS_FLOOR(minH), h);
        height += h + extraH;
        updateIfLarger(width, w + extraW);
    }
    if (pango_layout_get_character_count(lowerLayout_.get())) {
        pango_layout_get_pixel_size(lowerLayout_.get(), &w, &h);
        height += h + extraH;
        updateIfLarger(width, w + extraW);
    }

    bool vertical = parent_->config().verticalCandidateList.value();
    if (layoutHint_ == CandidateLayoutHint::Vertical) {
        vertical = true;
    } else if (layoutHint_ == CandidateLayoutHint::Horizontal) {
        vertical = false;
    }

    size_t wholeH = 0, wholeW = 0;
    for (size_t i = 0; i < nCandidates_; i++) {
        size_t candidateW = 0, candidateH = 0;
        if (pango_layout_get_character_count(labelLayouts_[i].get())) {
            pango_layout_get_pixel_size(labelLayouts_[i].get(), &w, &h);
            candidateW += w;
            updateIfLarger(candidateH, h + extraH);
        }
        if (pango_layout_get_character_count(candidateLayouts_[i].get())) {
            pango_layout_get_pixel_size(candidateLayouts_[i].get(), &w, &h);
            candidateW += w;
            updateIfLarger(candidateH, h + extraH);
        }
        candidateW += extraW;

        if (vertical) {
            wholeH += candidateH;
            updateIfLarger(wholeW, candidateW);
        } else {
            wholeW += candidateW;
            updateIfLarger(wholeH, candidateH);
        }
    }
    updateIfLarger(width, wholeW);
    candidatesHeight_ = wholeH;
    height += wholeH;
    const auto &margin = *parent_->theme().inputPanel->contentMargin;
    width += *margin.marginLeft + *margin.marginRight;
    height += *margin.marginTop + *margin.marginBottom;
    return {width, height};
}

static void prepareLayout(cairo_t *cr, PangoLayout *layout) {
    const PangoMatrix *matrix;

    matrix = pango_context_get_matrix(pango_layout_get_context(layout));

    if (matrix) {
        cairo_matrix_t cairo_matrix;

        cairo_matrix_init(&cairo_matrix, matrix->xx, matrix->yx, matrix->xy,
                          matrix->yy, matrix->x0, matrix->y0);

        cairo_transform(cr, &cairo_matrix);
    }
}

static void renderLayout(cairo_t *cr, PangoLayout *layout) {
    cairo_save(cr);
    prepareLayout(cr, layout);
    pango_cairo_show_layout(cr, layout);

    cairo_restore(cr);
}

void InputWindow::paint(cairo_t *cr, unsigned int width, unsigned int height) {
    auto &theme = parent_->theme();
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    theme.paint(cr, *theme.inputPanel->background, width, height);
    const auto &margin = *theme.inputPanel->contentMargin;
    const auto &textMargin = *theme.inputPanel->textMargin;

    // Move position to the right place.
    cairo_translate(cr, *margin.marginLeft, *margin.marginTop);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_save(cr);
    cairoSetSourceColor(cr, *theme.inputPanel->normalColor);
    // CLASSICUI_DEBUG() << theme.inputPanel->normalColor->toString();
    auto metrics = pango_context_get_metrics(
        context_.get(), pango_context_get_font_description(context_.get()),
        pango_context_get_language(context_.get()));
    auto minH = pango_font_metrics_get_ascent(metrics) +
                pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);

    size_t currentHeight = 0;
    int w, h;
    auto extraW = *textMargin.marginLeft + *textMargin.marginRight;
    auto extraH = *textMargin.marginTop + *textMargin.marginBottom;
    if (pango_layout_get_character_count(upperLayout_.get())) {
        cairo_move_to(cr, *textMargin.marginLeft, *textMargin.marginTop);
        renderLayout(cr, upperLayout_.get());
        pango_layout_get_pixel_size(upperLayout_.get(), &w, &h);
        h = std::max(PANGO_PIXELS_FLOOR(minH), h);
        PangoRectangle pos;
        if (cursor_ >= 0) {
            pango_layout_get_cursor_pos(upperLayout_.get(), cursor_, &pos,
                                        nullptr);

            cairo_save(cr);
            cairo_set_line_width(cr, 2);
            auto offsetX = pango_units_to_double(pos.x);
            cairo_move_to(cr, *textMargin.marginLeft + offsetX + 0.5,
                          *textMargin.marginTop);
            cairo_line_to(cr, *textMargin.marginLeft + offsetX + 0.5,
                          *textMargin.marginTop + h);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
        currentHeight += h + extraH;
    }
    if (pango_layout_get_character_count(lowerLayout_.get())) {
        cairo_move_to(cr, *textMargin.marginLeft,
                      *textMargin.marginTop + currentHeight);
        renderLayout(cr, lowerLayout_.get());
        pango_layout_get_pixel_size(lowerLayout_.get(), &w, &h);
        currentHeight += h + extraH;
    }

    bool vertical = parent_->config().verticalCandidateList.value();
    if (layoutHint_ == CandidateLayoutHint::Vertical) {
        vertical = true;
    } else if (layoutHint_ == CandidateLayoutHint::Horizontal) {
        vertical = false;
    }

    candidateRegions_.clear();
    candidateRegions_.reserve(nCandidates_);
    size_t wholeW = 0, wholeH = 0;
    for (size_t i = 0; i < nCandidates_; i++) {
        int x, y;
        if (vertical) {
            x = 0;
            y = currentHeight + wholeH;
        } else {
            x = wholeW;
            y = currentHeight;
        }
        x += *textMargin.marginLeft;
        y += *textMargin.marginTop;
        int labelW = 0, labelH = 0, candidateW = 0, candidateH = 0;
        if (pango_layout_get_character_count(labelLayouts_[i].get())) {
            pango_layout_get_pixel_size(labelLayouts_[i].get(), &labelW,
                                        &labelH);
        }
        if (pango_layout_get_character_count(candidateLayouts_[i].get())) {
            pango_layout_get_pixel_size(candidateLayouts_[i].get(), &candidateW,
                                        &candidateH);
        }
        int vheight;
        if (vertical) {
            vheight = std::max(labelH, candidateH);
            wholeH += vheight + extraH;
        } else {
            vheight = candidatesHeight_ - extraH;
            wholeW += candidateW + labelW + extraW;
        }
        const auto &highlightMargin = *theme.inputPanel->highlight->margin;
        const auto &clickMargin = *theme.inputPanel->highlight->clickMargin;
        auto highlightWidth = labelW + candidateW;
        if (*theme.inputPanel->fullWidthHighlight && vertical) {
            // Last candidate, fill.
            highlightWidth = width - *margin.marginLeft - *margin.marginRight -
                             *textMargin.marginRight - *textMargin.marginLeft;
            CLASSICUI_DEBUG() << width << " "
                              << highlightWidth + *highlightMargin.marginLeft +
                                     *highlightMargin.marginRight;
        }
        int highlightIndex = (hoverIndex_ > 0) ? hoverIndex_ : candidateIndex_;
        if (highlightIndex >= 0 && i == static_cast<size_t>(highlightIndex)) {
            cairo_save(cr);
            cairo_translate(cr, x - *highlightMargin.marginLeft,
                            y - *highlightMargin.marginTop);
            theme.paint(cr, *theme.inputPanel->highlight,
                        highlightWidth + *highlightMargin.marginLeft +
                            *highlightMargin.marginRight,
                        vheight + *highlightMargin.marginTop +
                            *highlightMargin.marginBottom);
            cairo_restore(cr);
            pango_layout_set_attributes(labelLayouts_[i].get(),
                                        highlightLabelAttrLists_[i].get());
            pango_layout_set_attributes(candidateLayouts_[i].get(),
                                        highlightCandidateAttrLists_[i].get());
        } else {
            pango_layout_set_attributes(labelLayouts_[i].get(),
                                        labelAttrLists_[i].get());

            pango_layout_set_attributes(candidateLayouts_[i].get(),
                                        candidateAttrLists_[i].get());
        }
        Rect candidateRegion;
        candidateRegion
            .setPosition(
                x - *highlightMargin.marginLeft + *clickMargin.marginLeft,
                y - *highlightMargin.marginTop + *clickMargin.marginTop)
            .setSize(highlightWidth + *highlightMargin.marginLeft +
                         *highlightMargin.marginRight -
                         *clickMargin.marginLeft - *clickMargin.marginRight,
                     vheight + *highlightMargin.marginTop +
                         *highlightMargin.marginBottom -
                         *clickMargin.marginTop - *clickMargin.marginBottom);
        candidateRegions_.push_back(candidateRegion);
        if (pango_layout_get_character_count(labelLayouts_[i].get())) {
            cairo_move_to(cr, x, y + (vheight - labelH) / 2.0);
            renderLayout(cr, labelLayouts_[i].get());
        }
        if (pango_layout_get_character_count(candidateLayouts_[i].get())) {
            cairo_move_to(cr, x + labelW, y + (vheight - candidateH) / 2.0);
            renderLayout(cr, candidateLayouts_[i].get());
        }
    }
    cairo_restore(cr);
}

void InputWindow::click(int x, int y) {
    auto inputContext = inputContext_.get();
    if (!inputContext) {
        return;
    }
    const auto *candidateList = inputContext->inputPanel().candidateList();
    if (!candidateList) {
        return;
    }
    for (size_t idx = 0, e = candidateRegions_.size(); idx < e; idx++) {
        if (candidateRegions_[idx].contains(x, y)) {
            auto candidate = nthCandidateIgnorePlaceholder(*candidateList, idx);
            if (candidate) {
                candidate->select(inputContext);
            }
            break;
        }
    }
}

int InputWindow::highlight() const {
    int highlightIndex = (hoverIndex_ > 0) ? hoverIndex_ : candidateIndex_;
    return highlightIndex;
}

void InputWindow::hover(int x, int y) {
    hoverIndex_ = -1;
    for (int idx = 0, e = candidateRegions_.size(); idx < e; idx++) {
        if (candidateRegions_[idx].contains(x, y)) {
            hoverIndex_ = idx;
            break;
        }
    }
}

} // namespace classicui
} // namespace fcitx
