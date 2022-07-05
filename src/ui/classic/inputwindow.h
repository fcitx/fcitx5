/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_INPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_INPUTWINDOW_H_

#include <utility>
#include <cairo/cairo.h>
#include <pango/pango.h>
#include "fcitx/candidatelist.h"
#include "fcitx/inputcontext.h"
#include "common.h"

namespace fcitx {
namespace classicui {

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
    int characterCount() const {
        int count = 0;
        for (const auto &layout : lines_) {
            count += pango_layout_get_character_count(layout.get());
        }
        return count;
    }

    int width() const;

    int size() { return lines_.size(); }
    void render(cairo_t *cr, int x, int y, int lineHeight, bool highlight);

    std::vector<GObjectUniquePtr<PangoLayout>> lines_;
    std::vector<PangoAttrListUniquePtr> attrLists_;
    std::vector<PangoAttrListUniquePtr> highlightAttrLists_;
};

class InputWindow {
public:
    InputWindow(ClassicUI *parent);
    std::pair<int, int> update(InputContext *inputContext);
    void paint(cairo_t *cr, unsigned int width, unsigned int height);
    void hide();
    bool visible() const { return visible_; }
    bool hover(int x, int y);
    void click(int x, int y);
    void wheel(bool up);

protected:
    void resizeCandidates(size_t n);
    void appendText(std::string &s, PangoAttrList *attrList,
                    PangoAttrList *highlightAttrList, const Text &text);
    void insertAttr(PangoAttrList *attrList, TextFormatFlags format, int start,
                    int end, bool highlight) const;
    void setTextToLayout(
        InputContext *inputContext, PangoLayout *layout,
        PangoAttrListUniquePtr *attrList,
        PangoAttrListUniquePtr *highlightAttrList,
        std::initializer_list<std::reference_wrapper<const Text>> texts);
    void setTextToMultilineLayout(InputContext *inputContext,
                                  MultilineLayout &layout, const Text &text);
    int highlight() const;

    ClassicUI *parent_;
    GObjectUniquePtr<PangoFontMap> fontMap_;
    double fontMapDefaultDPI_ = 96.0;
    GObjectUniquePtr<PangoContext> context_;
    GObjectUniquePtr<PangoLayout> upperLayout_;
    GObjectUniquePtr<PangoLayout> lowerLayout_;
    std::vector<MultilineLayout> labelLayouts_;
    std::vector<MultilineLayout> candidateLayouts_;
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
    size_t candidatesHeight_ = 0;
    int hoverIndex_ = -1;

private:
    std::pair<unsigned int, unsigned int> sizeHint();
};

} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_INPUTWINDOW_H_
