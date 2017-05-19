/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#include "inputwindow.h"
#include "classicui.h"
#include "fcitx-utils/color.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include <functional>
#include <initializer_list>
#include <limits>
#include <pango/pangocairo.h>

namespace fcitx {

namespace classicui {

void appendText(std::string &s, PangoAttrList *attrList, const Text &text) {
    for (size_t i = 0, e = text.size(); i < e; i++) {
        auto start = s.size();
        s.append(text.stringAt(i));
        auto end = s.size();
        if (start == end) {
            continue;
        }
        const auto format = text.formatAt(i);
        if (format & TextFormatFlag::UnderLine) {
            auto attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
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
        if (format & TextFormatFlag::HighLight) {
            auto scale = std::numeric_limits<unsigned short>::max();
            Color foreground("#ffffffff");
            Color background("#43ace8ff");
            auto attr = pango_attr_foreground_new(foreground.redF() * scale,
                                                  foreground.greenF() * scale,
                                                  foreground.blueF() * scale);
            attr->start_index = start;
            attr->end_index = end;
            pango_attr_list_insert(attrList, attr);
            attr = pango_attr_background_new(background.redF() * scale,
                                             background.greenF() * scale,
                                             background.blueF() * scale);
            attr->start_index = start;
            attr->end_index = end;
            pango_attr_list_insert(attrList, attr);
        }
    }
}

InputWindow::InputWindow(ClassicUI *parent)
    : parent_(parent), context_(nullptr, &g_object_unref),
      upperLayout_(nullptr, &g_object_unref),
      lowerLayout_(nullptr, &g_object_unref) {
    auto fontMap = pango_cairo_font_map_get_default();
    context_.reset(pango_font_map_create_context(fontMap));
    upperLayout_.reset(pango_layout_new(context_.get()));
    lowerLayout_.reset(pango_layout_new(context_.get()));
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

    nCandidates_ = n;
}

void setTextToLayout(
    PangoLayout *layout,
    std::initializer_list<std::reference_wrapper<const Text>> texts) {
    auto attrList = pango_attr_list_new();
    std::string line;
    for (auto &text : texts) {
        appendText(line, attrList, text);
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

    cursor_ = -1;
    auto preedit = instance->outputFilter(inputContext, inputPanel.preedit());
    auto auxUp = instance->outputFilter(inputContext, inputPanel.auxUp());
    pango_layout_set_single_paragraph_mode(upperLayout_.get(), true);
    setTextToLayout(upperLayout_.get(), {auxUp, preedit});
    if (preedit.cursor() >= 0) {
        cursor_ = preedit.cursor() + auxUp.toString().size();
    }

    auto auxDown = instance->outputFilter(inputContext, inputPanel.auxDown());
    setTextToLayout(lowerLayout_.get(), {auxDown});

    if (auto candidateList = inputPanel.candidateList()) {
        resizeCandidates(candidateList->size());

        for (int i = 0, e = candidateList->size(); i < e; i++) {
            auto label =
                instance->outputFilter(inputContext, candidateList->label(i));
            setTextToLayout(labelLayouts_[i].get(), {label});
            auto candidate = instance->outputFilter(
                inputContext, candidateList->candidate(i)->text());
            setTextToLayout(candidateLayouts_[i].get(), {candidate});
        }

        layoutHint_ = candidateList->layoutHint();
    } else {
        nCandidates_ = 0;
    }

    visible_ = nCandidates_ ||
               pango_layout_get_character_count(upperLayout_.get()) ||
               pango_layout_get_character_count(lowerLayout_.get());
}

std::pair<unsigned int, unsigned int> InputWindow::sizeHint() {
    auto fontDesc = pango_font_description_from_string("Sans 9");
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
    auto spaceWidth = pango_font_metrics_get_approximate_char_width(metrics);
    pango_font_metrics_unref(metrics);

    size_t width = 0;
    size_t height = 0;
    auto updateIfLarger = [](size_t &m, size_t n) {
        if (n > m) {
            m = n;
        }
    };
    int w, h;
    if (pango_layout_get_character_count(upperLayout_.get())) {

        pango_layout_get_pixel_size(upperLayout_.get(), &w, &h);
        h = std::max(PANGO_PIXELS_FLOOR(minH), h);
        height += h;
        updateIfLarger(width, w);
    }
    if (pango_layout_get_character_count(lowerLayout_.get())) {
        pango_layout_get_pixel_size(lowerLayout_.get(), &w, &h);
        height += h;
        updateIfLarger(width, w);
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
            updateIfLarger(candidateH, h);
        }
        if (pango_layout_get_character_count(candidateLayouts_[i].get())) {
            pango_layout_get_pixel_size(candidateLayouts_[i].get(), &w, &h);
            candidateW += w;
            updateIfLarger(candidateH, h);
        }

        if (vertical) {
            wholeH += candidateH;
            updateIfLarger(wholeW, candidateW);
        } else {
            wholeW += candidateW;
            if (i != 0) {
                wholeW += PANGO_PIXELS_FLOOR(spaceWidth);
            }
            updateIfLarger(wholeH, candidateH);
        }
    }
    updateIfLarger(width, wholeW);
    candidatesHeight_ = wholeH;
    height += wholeH;
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

void InputWindow::paint(cairo_t *cr) const {
    // FIXME background
    cairo_save(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_restore(cr);

    cairo_save(cr);
    // FIXME
    cairo_set_source_rgb(cr, 0, 0, 0);
    auto metrics = pango_context_get_metrics(
        context_.get(), pango_context_get_font_description(context_.get()),
        pango_context_get_language(context_.get()));
    auto minH = pango_font_metrics_get_ascent(metrics) +
                pango_font_metrics_get_descent(metrics);
    auto spaceWidth = pango_font_metrics_get_approximate_char_width(metrics);
    pango_font_metrics_unref(metrics);

    size_t height = 0;
    int w, h;
    if (pango_layout_get_character_count(upperLayout_.get())) {
        renderLayout(cr, upperLayout_.get());
        pango_layout_get_pixel_size(upperLayout_.get(), &w, &h);
        h = std::max(PANGO_PIXELS_FLOOR(minH), h);
        PangoRectangle pos;
        if (cursor_ >= 0) {
            pango_layout_get_cursor_pos(upperLayout_.get(), cursor_, &pos,
                                        nullptr);

            cairo_set_line_width(cr, 1);
            cairo_move_to(cr, pango_units_to_double(pos.x) + 0.5, 0);
            cairo_line_to(cr, pango_units_to_double(pos.x) + 0.5, h);
            cairo_stroke(cr);
        }
        height += h;
    }
    if (pango_layout_get_character_count(lowerLayout_.get())) {
        cairo_move_to(cr, 0, height);
        renderLayout(cr, lowerLayout_.get());
        pango_layout_get_pixel_size(lowerLayout_.get(), &w, &h);
        height += h;
    }

    cairo_move_to(cr, 0, height);

    bool vertical = parent_->config().verticalCandidateList.value();
    if (layoutHint_ == CandidateLayoutHint::Vertical) {
        vertical = true;
    } else if (layoutHint_ == CandidateLayoutHint::Horizontal) {
        vertical = false;
    }

    size_t wholeW = 0, wholeH = 0;
    for (size_t i = 0; i < nCandidates_; i++) {
        int x, y;
        if (vertical) {
            x = 0;
            y = height + wholeH;
        } else {
            if (i != 0) {
                wholeW += PANGO_PIXELS_FLOOR(spaceWidth);
            }
            x = wholeW;
            y = height;
        }
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
        } else {
            vheight = candidatesHeight_;

            wholeW += candidateW + labelW;
        }
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
}
}
