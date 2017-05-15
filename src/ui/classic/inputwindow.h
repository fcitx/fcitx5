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
#ifndef _FCITX_UI_CLASSIC_INPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_INPUTWINDOW_H_

#include "fcitx/candidatelist.h"
#include "fcitx/inputcontext.h"
#include <cairo/cairo.h>
#include <pango/pango.h>
#include <utility>

namespace fcitx {
namespace classicui {

class ClassicUI;

class InputWindow {
public:
    InputWindow(ClassicUI *parent);
    void update(InputContext *inputContext);
    std::pair<unsigned int, unsigned int> sizeHint();
    void paint(cairo_t *cr) const;
    void hide();
    bool visible() const { return visible_; }

protected:
    void resizeCandidates(size_t s);

    ClassicUI *parent_;
    std::unique_ptr<PangoContext, decltype(&g_object_unref)> context_;
    std::unique_ptr<PangoLayout, decltype(&g_object_unref)> upperLayout_;
    std::unique_ptr<PangoLayout, decltype(&g_object_unref)> lowerLayout_;
    std::vector<std::unique_ptr<PangoLayout, decltype(&g_object_unref)>>
        labelLayouts_;
    std::vector<std::unique_ptr<PangoLayout, decltype(&g_object_unref)>>
        candidateLayouts_;
    bool visible_ = false;
    int cursor_ = 0;
    int dpi_ = -1;
    size_t nCandidates_ = 0;
    CandidateLayoutHint layoutHint_;
    size_t candidatesHeight_;
};
}
}

#endif // _FCITX_UI_CLASSIC_INPUTWINDOW_H_
