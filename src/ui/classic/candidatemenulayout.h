/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_UI_CLASSIC_CANDIDATEMENULAYOUT_H_
#define _FCITX_UI_CLASSIC_CANDIDATEMENULAYOUT_H_

#include <cstddef>
#include <vector>
#include <yoga/YGNode.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/rect.h"

namespace fcitx::classicui {

class CandidateMenuLayout {
public:
    struct Margins {
        int left = 0;
        int right = 0;
        int top = 0;
        int bottom = 0;
    };

    struct Style {
        Margins content;
        Margins text;
        Margins highlight;
        int spacing = 0;
        int checkWidth = 0;
        int checkHeight = 0;
        int separatorHeight = 0;
    };

    struct Item {
        int textWidth;
        int textHeight;
        bool separator;
    };

    struct Row {
        Rect region;
        Rect check;
        Rect text;
        bool separator;
    };

    CandidateMenuLayout();
    void update(const Style &style, const std::vector<Item> &items);
    int width() const { return width_; }
    int height() const { return height_; }
    const Row &row(size_t index) const { return rows_.at(index); }
    int indexAt(int x, int y) const;

private:
    std::vector<Row> rows_;
    Margins highlight_;
    std::vector<UniqueCPtr<YGNode, YGNodeFree>> itemNodes_;
    UniqueCPtr<YGNode, YGNodeFree> rootNode_;
    int width_ = 0;
    int height_ = 0;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_CANDIDATEMENULAYOUT_H_
