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
#ifndef _FCITX_UI_CLASSIC_INPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_INPUTWINDOW_H_

#include "common.h"
#include "fcitx/candidatelist.h"
#include "fcitx/inputcontext.h"
#include <cairo/cairo.h>
#include <pango/pango.h>
#include <utility>

namespace fcitx {
namespace classicui {

class ClassicUI;

using PangoAttrListUniquePtr =
    std::unique_ptr<PangoAttrList, decltype(&pango_attr_list_unref)>;

class InputWindow {
public:
    InputWindow(ClassicUI *parent);
    void update(InputContext *inputContext);
    std::pair<unsigned int, unsigned int> sizeHint();
    void paint(cairo_t *cr, unsigned int width, unsigned int height);
    void hide();
    bool visible() const { return visible_; }
    void hover(int x, int y);
    void click(int x, int y);

protected:
    void resizeCandidates(size_t s);
    void appendText(std::string &s, PangoAttrList *attrList,
                    PangoAttrList *highlightAttrList, const Text &text);
    void insertAttr(PangoAttrList *attrList, TextFormatFlags format, int start,
                    int end, bool highlight) const;
    void setTextToLayout(
        PangoLayout *layout, PangoAttrListUniquePtr *attrList,
        PangoAttrListUniquePtr *highlightAttrList,
        std::initializer_list<std::reference_wrapper<const Text>> texts);
    int highlight() const;

    ClassicUI *parent_;
    GObjectUniquePtr<PangoContext> context_;
    GObjectUniquePtr<PangoLayout> upperLayout_;
    GObjectUniquePtr<PangoLayout> lowerLayout_;
    std::vector<GObjectUniquePtr<PangoLayout>> labelLayouts_;
    std::vector<GObjectUniquePtr<PangoLayout>> candidateLayouts_;
    std::vector<PangoAttrListUniquePtr> labelAttrLists_;
    std::vector<PangoAttrListUniquePtr> candidateAttrLists_;
    std::vector<PangoAttrListUniquePtr> highlightLabelAttrLists_;
    std::vector<PangoAttrListUniquePtr> highlightCandidateAttrLists_;
    std::vector<Rect> candidateRegions_;
    TrackableObjectReference<InputContext> inputContext_;
    bool visible_ = false;
    int cursor_ = 0;
    int dpi_ = -1;
    size_t nCandidates_ = 0;
    int candidateIndex_ = -1;
    CandidateLayoutHint layoutHint_ = CandidateLayoutHint::NotSet;
    size_t candidatesHeight_ = 0;
    int hoverIndex_ = -1;
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_INPUTWINDOW_H_
