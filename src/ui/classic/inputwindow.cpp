/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "inputwindow.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <string>
#include <tuple>
#include <utility>
#include <cairo.h>
#include <pango/pango-attributes.h>
#include <pango/pango-context.h>
#include <pango/pango-font.h>
#include <pango/pango-fontmap.h>
#include <pango/pango-language.h>
#include <pango/pango-layout.h>
#include <pango/pango-matrix.h>
#include <pango/pango-types.h>
#include <pango/pangocairo.h>
#include "fcitx-utils/color.h"
#include "fcitx-utils/rect.h"
#include "fcitx-utils/textformatflags.h"
#include "fcitx/candidatelist.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "fcitx/text.h"
#include "fcitx/userinterface.h"
#include "classicui.h"
#include "common.h"
#include "theme.h"

namespace fcitx::classicui {

auto newPangoLayout(PangoContext *context) {
    GObjectUniquePtr<PangoLayout> ptr(pango_layout_new(context));
    pango_layout_set_single_paragraph_mode(ptr.get(), false);
    return ptr;
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

static void renderLayout(cairo_t *cr, PangoLayout *layout, int x, int y) {
    auto *context = pango_layout_get_context(layout);
    auto *metrics = pango_context_get_metrics(
        context, pango_context_get_font_description(context),
        pango_context_get_language(context));
    auto ascent = pango_font_metrics_get_ascent(metrics);
    pango_font_metrics_unref(metrics);
    auto baseline = pango_layout_get_baseline(layout);
    auto yOffset = PANGO_PIXELS(ascent - baseline);
    cairo_save(cr);

    // Ensure the text are not painting on half pixel.
    cairo_move_to(cr, x, y + yOffset);
    double dx;
    double dy;
    double odx;
    double ody;
    cairo_get_current_point(cr, &dx, &dy);
    // Save old user value
    odx = dx;
    ody = dy;
    // Convert to device and round.
    cairo_user_to_device(cr, &dx, &dy);
    double ndx = std::round(dx);
    double ndy = std::round(dy);
    // Convert back to user and calculate delta.
    cairo_device_to_user(cr, &ndx, &ndy);
    cairo_move_to(cr, x + ndx - odx, y + yOffset + ndy - ody);

    prepareLayout(cr, layout);
    pango_cairo_show_layout(cr, layout);

    cairo_restore(cr);
}

int MultilineLayout::width() const {
    int width = 0;
    for (const auto &layout : lines_) {
        int w;
        int h;
        pango_layout_get_pixel_size(layout.get(), &w, &h);
        width = std::max(width, w);
    }
    return width;
}

void MultilineLayout::render(cairo_t *cr, int x, int y, int lineHeight,
                             bool highlight) {
    for (size_t i = 0; i < lines_.size(); i++) {
        if (highlight) {
            pango_layout_set_attributes(lines_[i].get(),
                                        highlightAttrLists_[i].get());
        } else {
            pango_layout_set_attributes(lines_[i].get(), attrLists_[i].get());
        }
        renderLayout(cr, lines_[i].get(), x, y);
        y += lineHeight;
    }
}

InputWindow::InputWindow(ClassicUI *parent) : parent_(parent) {
    fontMap_.reset(pango_cairo_font_map_new());
    // Although documentation says it is 96 by default, try not rely on this
    // behavior.
    fontMapDefaultDPI_ = pango_cairo_font_map_get_resolution(
        PANGO_CAIRO_FONT_MAP(fontMap_.get()));
    context_.reset(pango_font_map_create_context(fontMap_.get()));
    upperLayout_ = newPangoLayout(context_.get());
    lowerLayout_ = newPangoLayout(context_.get());
}

void InputWindow::insertAttr(PangoAttrList *attrList, TextFormatFlags format,
                             int start, int end, bool highlight) const {
    if (format & TextFormatFlag::Underline) {
        auto *attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);
    }
    if (format & TextFormatFlag::Italic) {
        auto *attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);
    }
    if (format & TextFormatFlag::Strike) {
        auto *attr = pango_attr_strikethrough_new(true);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);
    }
    if (format & TextFormatFlag::Bold) {
        auto *attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);
    }
    Color color =
        (format & TextFormatFlag::HighLight)
            ? parent_->theme().inputPanelHighlightText()
            : (highlight ? parent_->theme().inputPanelHighlightCandidateText()
                         : parent_->theme().inputPanelText());
    const auto scale = std::numeric_limits<uint16_t>::max();
    auto *attr = pango_attr_foreground_new(
        color.redF() * scale, color.greenF() * scale, color.blueF() * scale);
    attr->start_index = start;
    attr->end_index = end;
    pango_attr_list_insert(attrList, attr);

    if (color.alpha() != 255) {
        auto *alphaAttr =
            pango_attr_foreground_alpha_new(color.alphaF() * scale);
        alphaAttr->start_index = start;
        alphaAttr->end_index = end;
        pango_attr_list_insert(attrList, alphaAttr);
    }

    auto background = parent_->theme().inputPanelHighlight();
    if (format.test(TextFormatFlag::HighLight) && background.alpha() > 0) {
        attr = pango_attr_background_new(background.redF() * scale,
                                         background.greenF() * scale,
                                         background.blueF() * scale);
        attr->start_index = start;
        attr->end_index = end;
        pango_attr_list_insert(attrList, attr);

        if (background.alpha() != 255) {
            auto *alphaAttr =
                pango_attr_background_alpha_new(background.alphaF() * scale);
            alphaAttr->start_index = start;
            alphaAttr->end_index = end;
            pango_attr_list_insert(attrList, alphaAttr);
        }
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
        labelLayouts_.emplace_back();
    }
    while (candidateLayouts_.size() < n) {
        candidateLayouts_.emplace_back();
    }

    nCandidates_ = n;
}

void InputWindow::setTextToMultilineLayout(InputContext *inputContext,
                                           MultilineLayout &layout,
                                           const Text &text) {
    auto lines = text.splitByLine();
    layout.lines_.clear();
    layout.attrLists_.clear();
    layout.highlightAttrLists_.clear();

    for (const auto &line : lines) {
        layout.lines_.emplace_back(pango_layout_new(context_.get()));
        layout.attrLists_.emplace_back();
        layout.highlightAttrLists_.emplace_back();
        setTextToLayout(inputContext, layout.lines_.back().get(),
                        &layout.attrLists_.back(),
                        &layout.highlightAttrLists_.back(), {line});
    }
}

void InputWindow::setTextToLayout(
    InputContext *inputContext, PangoLayout *layout,
    PangoAttrListUniquePtr *attrList, PangoAttrListUniquePtr *highlightAttrList,
    std::initializer_list<std::reference_wrapper<const Text>> texts) {
    auto *newAttrList = pango_attr_list_new();
    if (attrList) {
        // PangoAttrList does not have "clear()". So when we set new text,
        // we need to create a new one and get rid of old one.
        // We keep a ref to the attrList.
        attrList->reset(pango_attr_list_ref(newAttrList));
    }
    PangoAttrList *newHighlightAttrList = nullptr;
    if (highlightAttrList) {
        newHighlightAttrList = pango_attr_list_new();
        highlightAttrList->reset(newHighlightAttrList);
    }
    std::string line;
    for (const auto &text : texts) {
        appendText(line, newAttrList, newHighlightAttrList, text);
    }

    const auto *entry = parent_->instance()->inputMethodEntry(inputContext);
    if (*parent_->config().useInputMethodLanguageToDisplayText && entry &&
        !entry->languageCode().empty()) {
        if (auto *language =
                pango_language_from_string(entry->languageCode().c_str())) {
            if (newAttrList) {
                auto *attr = pango_attr_language_new(language);
                attr->start_index = 0;
                attr->end_index = line.size();
                pango_attr_list_insert(newAttrList, attr);
            }
            if (newHighlightAttrList) {
                auto *attr = pango_attr_language_new(language);
                attr->start_index = 0;
                attr->end_index = line.size();
                pango_attr_list_insert(newHighlightAttrList, attr);
            }
        }
    }

    pango_layout_set_text(layout, line.c_str(), line.size());
    pango_layout_set_attributes(layout, newAttrList);
    pango_attr_list_unref(newAttrList);
}

std::pair<int, int> InputWindow::update(InputContext *inputContext) {
    hoverIndex_ = -1;
    if ((parent_->suspended() &&
         parent_->instance()->currentUI() != "kimpanel") ||
        !inputContext) {
        visible_ = false;
        return {0, 0};
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
    auto *instance = parent_->instance();
    auto &inputPanel = inputContext->inputPanel();
    inputContext_ = inputContext->watch();

    cursor_ = -1;
    auto preedit = instance->outputFilter(inputContext, inputPanel.preedit());
    auto auxUp = instance->outputFilter(inputContext, inputPanel.auxUp());
    pango_layout_set_single_paragraph_mode(upperLayout_.get(), true);
    setTextToLayout(inputContext, upperLayout_.get(), nullptr, nullptr,
                    {auxUp, preedit});
    if (preedit.cursor() >= 0 &&
        static_cast<size_t>(preedit.cursor()) <= preedit.textLength()) {
        cursor_ = preedit.cursor() + auxUp.toString().size();
    }

    auto auxDown = instance->outputFilter(inputContext, inputPanel.auxDown());
    setTextToLayout(inputContext, lowerLayout_.get(), nullptr, nullptr,
                    {auxDown});

    if (auto candidateList = inputPanel.candidateList()) {
        // Count non-placeholder candidates.
        int count = 0;

        for (int i = 0, e = candidateList->size(); i < e; i++) {
            const auto &candidate = candidateList->candidate(i);
            if (candidate.isPlaceHolder()) {
                continue;
            }
            count++;
        }
        resizeCandidates(count);

        candidateIndex_ = -1;
        int localIndex = 0;
        for (int i = 0, e = candidateList->size(); i < e; i++) {
            const auto &candidate = candidateList->candidate(i);
            // Skip placeholder.
            if (candidate.isPlaceHolder()) {
                continue;
            }

            if (i == candidateList->cursorIndex()) {
                candidateIndex_ = localIndex;
            }

            Text labelText = candidate.hasCustomLabel()
                                 ? candidate.customLabel()
                                 : candidateList->label(i);

            labelText = instance->outputFilter(inputContext, labelText);
            setTextToMultilineLayout(inputContext, labelLayouts_[localIndex],
                                     labelText);
            auto candidateText = instance->outputFilter(
                inputContext, candidate.textWithComment());
            setTextToMultilineLayout(
                inputContext, candidateLayouts_[localIndex], candidateText);
            localIndex++;
        }

        layoutHint_ = candidateList->layoutHint();
        if (auto *pageable = candidateList->toPageable()) {
            hasPrev_ = pageable->hasPrev();
            hasNext_ = pageable->hasNext();
        } else {
            hasPrev_ = false;
            hasNext_ = false;
        }
    } else {
        nCandidates_ = 0;
        candidateIndex_ = -1;
        hasPrev_ = false;
        hasNext_ = false;
    }

    visible_ = nCandidates_ ||
               pango_layout_get_character_count(upperLayout_.get()) ||
               pango_layout_get_character_count(lowerLayout_.get());
    int width = 0;
    int height = 0;
    if (visible_) {
        std::tie(width, height) = sizeHint();
        if (width <= 0 || height <= 0) {
            width = height = 0;
            visible_ = false;
        }
    }
    return {width, height};
}

std::pair<unsigned int, unsigned int> InputWindow::sizeHint() {
    auto &theme = parent_->theme();
    auto *fontDesc =
        pango_font_description_from_string(parent_->config().font->c_str());
    pango_context_set_font_description(context_.get(), fontDesc);
    pango_font_description_free(fontDesc);
    pango_layout_context_changed(upperLayout_.get());
    pango_layout_context_changed(lowerLayout_.get());
    for (size_t i = 0; i < nCandidates_; i++) {
        labelLayouts_[i].contextChanged();
        candidateLayouts_[i].contextChanged();
    }
    auto *metrics = pango_context_get_metrics(
        context_.get(), pango_context_get_font_description(context_.get()),
        pango_context_get_language(context_.get()));
    auto fontHeight = pango_font_metrics_get_ascent(metrics) +
                      pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);
    fontHeight = PANGO_PIXELS(fontHeight);

    size_t width = 0;
    size_t height = 0;
    auto updateIfLarger = [](size_t &m, size_t n) {
        if (n > m) {
            m = n;
        }
    };
    int w;
    int h;

    const auto &textMargin = *theme.inputPanel->textMargin;
    auto extraW = *textMargin.marginLeft + *textMargin.marginRight;
    auto extraH = *textMargin.marginTop + *textMargin.marginBottom;
    if (pango_layout_get_character_count(upperLayout_.get())) {
        pango_layout_get_pixel_size(upperLayout_.get(), &w, &h);
        height += fontHeight + extraH;
        updateIfLarger(width, w + extraW);
    }
    if (pango_layout_get_character_count(lowerLayout_.get())) {
        pango_layout_get_pixel_size(lowerLayout_.get(), &w, &h);
        height += fontHeight + extraH;
        updateIfLarger(width, w + extraW);
    }

    bool vertical = parent_->config().verticalCandidateList.value();
    if (layoutHint_ == CandidateLayoutHint::Vertical) {
        vertical = true;
    } else if (layoutHint_ == CandidateLayoutHint::Horizontal) {
        vertical = false;
    }

    size_t wholeH = 0;
    size_t wholeW = 0;
    for (size_t i = 0; i < nCandidates_; i++) {
        size_t candidateW = 0;
        size_t candidateH = 0;
        if (labelLayouts_[i].characterCount()) {
            candidateW += labelLayouts_[i].width();
            updateIfLarger(candidateH,
                           std::max(1, labelLayouts_[i].size()) * fontHeight +
                               extraH);
        }
        if (candidateLayouts_[i].characterCount()) {
            candidateW += candidateLayouts_[i].width();
            updateIfLarger(
                candidateH,
                std::max(1, candidateLayouts_[i].size()) * fontHeight + extraH);
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
    const auto &margin = *theme.inputPanel->contentMargin;
    width += *margin.marginLeft + *margin.marginRight;
    height += *margin.marginTop + *margin.marginBottom;

    if (nCandidates_ && (hasPrev_ || hasNext_)) {
        const auto &prev = theme.loadAction(*theme.inputPanel->prev);
        const auto &next = theme.loadAction(*theme.inputPanel->next);
        if (prev.valid() && next.valid()) {
            width += prev.width() + next.width();
        }
    }

    return {width, height};
}

void InputWindow::paint(cairo_t *cr, unsigned int width, unsigned int height,
                        double scale) {
    cairo_scale(cr, scale, scale);
    auto &theme = parent_->theme();
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    theme.paint(cr, *theme.inputPanel->background, width, height, /*alpha=*/1.0,
                scale);
    const auto &margin = *theme.inputPanel->contentMargin;
    const auto &textMargin = *theme.inputPanel->textMargin;
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_save(cr);

    // Move position to the right place.
    cairo_translate(cr, *margin.marginLeft, *margin.marginTop);
    cairoSetSourceColor(cr, theme.inputPanelText());
    // CLASSICUI_DEBUG() << theme.inputPanel->normalColor->toString();
    auto *metrics = pango_context_get_metrics(
        context_.get(), pango_context_get_font_description(context_.get()),
        pango_context_get_language(context_.get()));
    auto fontHeight = pango_font_metrics_get_ascent(metrics) +
                      pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);
    fontHeight = PANGO_PIXELS(fontHeight);

    size_t currentHeight = 0;
    int w;
    int h;
    auto extraW = *textMargin.marginLeft + *textMargin.marginRight;
    auto extraH = *textMargin.marginTop + *textMargin.marginBottom;
    if (pango_layout_get_character_count(upperLayout_.get())) {
        renderLayout(cr, upperLayout_.get(), *textMargin.marginLeft,
                     *textMargin.marginTop);
        pango_layout_get_pixel_size(upperLayout_.get(), &w, &h);
        PangoRectangle pos;
        if (cursor_ >= 0) {
            pango_layout_get_cursor_pos(upperLayout_.get(), cursor_, &pos,
                                        nullptr);

            cairo_save(cr);
            cairo_set_line_width(cr, 2);
            auto offsetX = pango_units_to_double(pos.x);
            cairo_move_to(cr, *textMargin.marginLeft + offsetX + 1,
                          *textMargin.marginTop);
            cairo_line_to(cr, *textMargin.marginLeft + offsetX + 1,
                          *textMargin.marginTop + fontHeight);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
        currentHeight += fontHeight + extraH;
    }
    if (pango_layout_get_character_count(lowerLayout_.get())) {
        renderLayout(cr, lowerLayout_.get(), *textMargin.marginLeft,
                     *textMargin.marginTop + currentHeight);
        pango_layout_get_pixel_size(lowerLayout_.get(), &w, nullptr);
        currentHeight += fontHeight + extraH;
    }

    bool vertical = parent_->config().verticalCandidateList.value();
    if (layoutHint_ == CandidateLayoutHint::Vertical) {
        vertical = true;
    } else if (layoutHint_ == CandidateLayoutHint::Horizontal) {
        vertical = false;
    }

    candidateRegions_.clear();
    candidateRegions_.reserve(nCandidates_);
    size_t wholeW = 0;
    size_t wholeH = 0;

    // size of text = textMargin + actual text size.
    // HighLight = HighLight margin + TEXT.
    // Click region = HighLight - click

    for (size_t i = 0; i < nCandidates_; i++) {
        int x;
        int y;
        if (vertical) {
            x = 0;
            y = currentHeight + wholeH;
        } else {
            x = wholeW;
            y = currentHeight;
        }
        x += *textMargin.marginLeft;
        y += *textMargin.marginTop;
        int labelW = 0;
        int labelH = 0;
        int candidateW = 0;
        int candidateH = 0;
        if (labelLayouts_[i].characterCount()) {
            labelW = labelLayouts_[i].width();
            labelH = fontHeight * labelLayouts_[i].size();
        }
        if (candidateLayouts_[i].characterCount()) {
            candidateW = candidateLayouts_[i].width();
            candidateH = fontHeight * candidateLayouts_[i].size();
        }
        int vheight;
        if (vertical) {
            vheight = std::max({fontHeight, labelH, candidateH});
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
        }
        const int highlightIndex = highlight();
        bool highlight = false;
        if (highlightIndex >= 0 && i == static_cast<size_t>(highlightIndex)) {
            cairo_save(cr);
            cairo_translate(cr, x - *highlightMargin.marginLeft,
                            y - *highlightMargin.marginTop);
            theme.paint(cr, *theme.inputPanel->highlight,
                        highlightWidth + *highlightMargin.marginLeft +
                            *highlightMargin.marginRight,
                        vheight + *highlightMargin.marginTop +
                            *highlightMargin.marginBottom,
                        /*alpha=*/1.0, scale);
            cairo_restore(cr);
            highlight = true;
        }
        Rect candidateRegion;
        candidateRegion
            .setPosition(*margin.marginLeft + x - *highlightMargin.marginLeft +
                             *clickMargin.marginLeft,
                         *margin.marginTop + y - *highlightMargin.marginTop +
                             *clickMargin.marginTop)
            .setSize(highlightWidth + *highlightMargin.marginLeft +
                         *highlightMargin.marginRight -
                         *clickMargin.marginLeft - *clickMargin.marginRight,
                     vheight + *highlightMargin.marginTop +
                         *highlightMargin.marginBottom -
                         *clickMargin.marginTop - *clickMargin.marginBottom);
        candidateRegions_.push_back(candidateRegion);
        if (labelLayouts_[i].characterCount()) {
            labelLayouts_[i].render(cr, x, y, fontHeight, highlight);
        }
        if (candidateLayouts_[i].characterCount()) {
            candidateLayouts_[i].render(cr, x + labelW, y, fontHeight,
                                        highlight);
        }
    }
    cairo_restore(cr);

    prevRegion_ = Rect();
    nextRegion_ = Rect();
    if (nCandidates_ && (hasPrev_ || hasNext_)) {
        const auto &prev = theme.loadAction(*theme.inputPanel->prev);
        const auto &next = theme.loadAction(*theme.inputPanel->next);
        if (prev.valid() && next.valid()) {
            cairo_save(cr);
            int prevY = 0;
            int nextY = 0;
            switch (*theme.inputPanel->buttonAlignment) {
            case PageButtonAlignment::Top:
                prevY = *margin.marginTop;
                nextY = *margin.marginTop;
                break;
            case PageButtonAlignment::FirstCandidate:
                prevY =
                    candidateRegions_.front().top() +
                    (candidateRegions_.front().height() - prev.height()) / 2.0;
                ;
                nextY =
                    candidateRegions_.front().top() +
                    (candidateRegions_.front().height() - prev.height()) / 2.0;
                ;
                break;
            case PageButtonAlignment::Center:
                prevY =
                    *margin.marginTop + (height - *margin.marginTop -
                                         *margin.marginBottom - prev.height()) /
                                            2.0;
                nextY =
                    *margin.marginTop + (height - *margin.marginTop -
                                         *margin.marginBottom - next.height()) /
                                            2.0;
                break;
            case PageButtonAlignment::LastCandidate:
                prevY =
                    candidateRegions_.back().top() +
                    (candidateRegions_.back().height() - prev.height()) / 2.0;
                nextY =
                    candidateRegions_.back().top() +
                    (candidateRegions_.back().height() - next.height()) / 2.0;
                break;
            case PageButtonAlignment::Bottom:
            default:
                prevY = height - *margin.marginBottom - prev.height();
                nextY = height - *margin.marginBottom - next.height();
                break;
            }
            nextRegion_.setPosition(width - *margin.marginRight - next.width(),
                                    nextY);
            nextRegion_.setSize(next.width(), next.height());
            cairo_translate(cr, nextRegion_.left(), nextRegion_.top());
            shrink(nextRegion_, *theme.inputPanel->next->clickMargin);
            double alpha = 1.0;
            if (!hasNext_) {
                alpha = 0.3;
            } else if (nextHovered_) {
                alpha = 0.7;
            }
            theme.paint(cr, *theme.inputPanel->next, alpha);
            cairo_restore(cr);
            cairo_save(cr);
            prevRegion_.setPosition(width - *margin.marginRight - next.width() -
                                        prev.width(),
                                    prevY);
            prevRegion_.setSize(prev.width(), prev.height());
            cairo_translate(cr, prevRegion_.left(), prevRegion_.top());
            shrink(prevRegion_, *theme.inputPanel->prev->clickMargin);
            alpha = 1.0;
            if (!hasPrev_) {
                alpha = 0.3;
            } else if (prevHovered_) {
                alpha = 0.7;
            }
            theme.paint(cr, *theme.inputPanel->prev, alpha);
            cairo_restore(cr);
        }
    }
}

void InputWindow::click(int x, int y) {
    auto *inputContext = inputContext_.get();
    if (!inputContext) {
        return;
    }
    const auto candidateList = inputContext->inputPanel().candidateList();
    if (!candidateList) {
        return;
    }
    if (auto *pageable = candidateList->toPageable()) {
        if (pageable->hasPrev() && prevRegion_.contains(x, y)) {
            pageable->prev();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return;
        }
        if (pageable->hasNext() && nextRegion_.contains(x, y)) {
            pageable->next();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return;
        }
    }
    for (size_t idx = 0, e = candidateRegions_.size(); idx < e; idx++) {
        if (candidateRegions_[idx].contains(x, y)) {
            const auto *candidate =
                nthCandidateIgnorePlaceholder(*candidateList, idx);
            if (candidate) {
                candidate->select(inputContext);
            }
            break;
        }
    }
}

void InputWindow::wheel(bool up) {
    if (!*parent_->config().useWheelForPaging) {
        return;
    }
    auto *inputContext = inputContext_.get();
    if (!inputContext) {
        return;
    }
    const auto candidateList = inputContext->inputPanel().candidateList();
    if (!candidateList) {
        return;
    }
    if (auto *pageable = candidateList->toPageable()) {
        if (up) {
            if (pageable->hasPrev()) {
                pageable->prev();
                inputContext->updateUserInterface(
                    UserInterfaceComponent::InputPanel);
            }
        } else {
            if (pageable->hasNext()) {
                pageable->next();
                inputContext->updateUserInterface(
                    UserInterfaceComponent::InputPanel);
            }
        }
    }
}

void InputWindow::setFontDPI(int dpi) {
    // Unlike pango cairo context, Cairo font map does not accept negative dpi.
    // Restore to default value instead.
    if (dpi <= 0) {
        pango_cairo_font_map_set_resolution(
            PANGO_CAIRO_FONT_MAP(fontMap_.get()), fontMapDefaultDPI_);
    } else {
        pango_cairo_font_map_set_resolution(
            PANGO_CAIRO_FONT_MAP(fontMap_.get()), dpi);
    }
    pango_cairo_context_set_resolution(context_.get(), dpi);
}

int InputWindow::highlight() const {
    int highlightIndex = (hoverIndex_ >= 0) ? hoverIndex_ : candidateIndex_;
    return highlightIndex;
}

bool InputWindow::hover(int x, int y) {
    bool needRepaint = false;

    bool prevHovered = false;
    bool nextHovered = false;
    auto oldHighlight = highlight();
    hoverIndex_ = -1;

    prevHovered = prevRegion_.contains(x, y);
    if (!prevHovered) {
        nextHovered = nextRegion_.contains(x, y);
        if (!nextHovered) {
            for (int idx = 0, e = candidateRegions_.size(); idx < e; idx++) {
                if (candidateRegions_[idx].contains(x, y)) {
                    hoverIndex_ = idx;
                    break;
                }
            }
        }
    }

    needRepaint = needRepaint || prevHovered_ != prevHovered;
    prevHovered_ = prevHovered;

    needRepaint = needRepaint || nextHovered_ != nextHovered;
    nextHovered_ = nextHovered;

    needRepaint = needRepaint || oldHighlight != highlight();
    return needRepaint;
}

} // namespace fcitx::classicui
