/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_INPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_INPUTWINDOW_H_

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>
#include <cairo.h>
#include <pango/pango-attributes.h>
#include <pango/pango-context.h>
#include <pango/pango-font.h>
#include <pango/pango-layout.h>
#include <pango/pango-types.h>
#include <pango/pango.h>
#include <yoga/YGConfig.h>
#include <yoga/YGNode.h>
#include <yoga/Yoga.h>
#include "fcitx-utils/macros.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/rect.h"
#include "fcitx-utils/textformatflags.h"
#include "fcitx-utils/trackableobject.h"
#include "fcitx/candidatelist.h"
#include "fcitx/inputcontext.h"
#include "fcitx/text.h"
#include "common.h"

namespace fcitx::classicui {

class ClassicUI;

using PangoAttrListUniquePtr = UniqueCPtr<PangoAttrList, pango_attr_list_unref>;

class MultilineLayout {
public:
    MultilineLayout() = default;
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(MultilineLayout);

    void contextChanged() {
        for (const auto &layout : lines_) {
            pango_layout_context_changed(layout.get());
        }
    }
    void setFontDescription(const PangoFontDescription *fontDesc) {
        for (const auto &layout : lines_) {
            pango_layout_set_font_description(layout.get(), fontDesc);
        }
    }
    int fontHeight() const {
        if (lines_.empty()) {
            return 0;
        }
        auto *context = pango_layout_get_context(lines_[0].get());
        if (!context) {
            return 0;
        }
        const auto *fontDescription =
            pango_layout_get_font_description(lines_[0].get());
        if (!fontDescription) {
            fontDescription = pango_context_get_font_description(context);
        }
        if (!fontDescription) {
            return 0;
        }
        auto *metrics = pango_context_get_metrics(
            context, fontDescription, pango_context_get_language(context));
        int fontHeight = pango_font_metrics_get_ascent(metrics) +
                         pango_font_metrics_get_descent(metrics);
        pango_font_metrics_unref(metrics);
        return PANGO_PIXELS(fontHeight);
    }
    int characterCount() const {
        int count = 0;
        for (const auto &layout : lines_) {
            count += pango_layout_get_character_count(layout.get());
        }
        return count;
    }

    int width() const;

    int size() const { return lines_.size(); }
    void render(cairo_t *cr, int x, int y, bool highlight);

    std::vector<GObjectUniquePtr<PangoLayout>> lines_;
    std::vector<PangoAttrListUniquePtr> attrLists_;
    std::vector<PangoAttrListUniquePtr> highlightAttrLists_;
};

class InputWindow {
public:
    InputWindow(ClassicUI *parent);
    ~InputWindow() = default;
    std::pair<int, int> update(InputContext *inputContext);
    void paint(cairo_t *cr, unsigned int width, unsigned int height,
               double scale);
    void hide();
    bool visible() const { return visible_; }
    bool hover(int x, int y);
    void click(int x, int y);
    void wheel(bool up);

protected:
    enum class TextType { Label, Regular, Comment };

    // Override the font DPI in cairo/pango context. If less-equal than zero, it
    // will restore to font map default dpi.
    void setFontDPI(int dpi);
    void resizeCandidates(size_t n);
    void appendText(std::string &s, PangoAttrList *attrList,
                    PangoAttrList *highlightAttrList, const Text &text,
                    TextType type);
    void insertAttr(PangoAttrList *attrList, TextFormatFlags format, int start,
                    int end, bool highlight, TextType type) const;
    void setTextToLayout(
        InputContext *inputContext, PangoLayout *layout,
        PangoAttrListUniquePtr *attrList,
        PangoAttrListUniquePtr *highlightAttrList,
        std::initializer_list<std::reference_wrapper<const Text>> texts,
        TextType type = TextType::Regular);
    void setTextToMultilineLayout(InputContext *inputContext,
                                  MultilineLayout &layout, const Text &text,
                                  TextType type);
    int highlight() const;
    void updateYogaLayout();
    void renderYogaNode(cairo_t *cr, YGNodeRef node);

    ClassicUI *parent_;
    GObjectUniquePtr<PangoFontMap> fontMap_;
    double fontMapDefaultDPI_ = 96.0;
    GObjectUniquePtr<PangoContext> context_;
    GObjectUniquePtr<PangoLayout> upperLayout_;
    GObjectUniquePtr<PangoLayout> lowerLayout_;
    struct CandidateLayout {
        MultilineLayout label;
        MultilineLayout text;
        MultilineLayout comment;
    };
    std::vector<CandidateLayout> candidateLayouts_;
    std::vector<Rect> candidateRegions_;
    TrackableObjectReference<InputContext> inputContext_;
    bool visible_ = false;
    int cursor_ = 0;
    size_t nCandidates_ = 0;
    bool hasPrev_ = false;
    bool hasNext_ = false;
    Rect prevRegion_;
    Rect nextRegion_;
    bool prevHovered_ = false;
    bool nextHovered_ = false;
    int candidateIndex_ = -1;
    CandidateLayoutHint layoutHint_ = CandidateLayoutHint::NotSet;
    int hoverIndex_ = -1;

private:
    std::pair<unsigned int, unsigned int> sizeHint();

    using YGNodePtr = fcitx::UniqueCPtr<YGNode, YGNodeFree>;
    YGNodePtr rootNode_;
    YGNodePtr mainNode_;
    YGNodePtr upperNode_;
    YGNodePtr upperTextNode_;
    YGNodePtr lowerNode_;
    YGNodePtr auxDownNode_;
    YGNodePtr auxDownTextNode_;
    YGNodePtr candidatesNode_;

    struct CandidateNode {
        YGNodePtr self{YGNodeNew()};
        YGNodePtr inner{YGNodeNew()};
        YGNodePtr label{YGNodeNew()};
        YGNodePtr text{YGNodeNew()};
        YGNodePtr comment{YGNodeNew()};
    };

    std::vector<CandidateNode> candidateNodes_;
    YGNodePtr buttonNode_;

    template <auto Getter>
    float absolute(const YGNodePtr &node) const {
        float offset = 0.0F;
        YGNodeRef current = node.get();
        while (current != nullptr) {
            offset += Getter(current);
            current = YGNodeGetParent(current);
        }
        return offset;
    }
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_INPUTWINDOW_H_
