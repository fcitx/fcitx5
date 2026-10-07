/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "candidatemenulayout.h"
#include <algorithm>
#include <yoga/YGNodeLayout.h>
#include <yoga/YGNodeStyle.h>
#include <yoga/Yoga.h>

namespace fcitx::classicui {

CandidateMenuLayout::CandidateMenuLayout() : rootNode_(YGNodeNew()) {}

void CandidateMenuLayout::update(const Style &style,
                                 const std::vector<Item> &items) {
    highlight_ = style.highlight;
    YGNodeRemoveAllChildren(rootNode_.get());
    itemNodes_.clear();
    rows_.clear();
    YGNodeReset(rootNode_.get());
    YGNodeStyleSetFlexDirection(rootNode_.get(), YGFlexDirectionColumn);
    YGNodeStyleSetPadding(rootNode_.get(), YGEdgeLeft, style.content.left);
    YGNodeStyleSetPadding(rootNode_.get(), YGEdgeRight, style.content.right);
    YGNodeStyleSetPadding(rootNode_.get(), YGEdgeTop, style.content.top);
    YGNodeStyleSetPadding(rootNode_.get(), YGEdgeBottom, style.content.bottom);
    YGNodeStyleSetGap(rootNode_.get(), YGGutterRow, style.spacing);
    YGNodeStyleSetMinWidth(rootNode_.get(), 1);
    YGNodeStyleSetMinHeight(rootNode_.get(), 1);

    int maxTextWidth = 0;
    int maxTextHeight = 0;
    for (const auto &item : items) {
        if (!item.separator) {
            maxTextWidth = std::max(maxTextWidth, item.textWidth);
            maxTextHeight = std::max(maxTextHeight, item.textHeight);
        }
    }
    const int itemWidth = maxTextWidth + style.checkWidth;
    const int itemHeight = std::max(maxTextHeight, style.checkHeight);
    itemNodes_.reserve(items.size());
    rows_.reserve(items.size());
    for (size_t i = 0; i < items.size(); i++) {
        itemNodes_.emplace_back(YGNodeNew());
        auto *node = itemNodes_.back().get();
        if (items[i].separator) {
            YGNodeStyleSetWidth(node,
                                itemWidth + style.text.left + style.text.right);
            YGNodeStyleSetHeight(node, style.separatorHeight);
        } else {
            YGNodeStyleSetWidth(node, itemWidth);
            YGNodeStyleSetHeight(node, itemHeight);
            YGNodeStyleSetMargin(node, YGEdgeLeft, style.text.left);
            YGNodeStyleSetMargin(node, YGEdgeRight, style.text.right);
            YGNodeStyleSetMargin(node, YGEdgeTop, style.text.top);
            YGNodeStyleSetMargin(node, YGEdgeBottom, style.text.bottom);
        }
        YGNodeInsertChild(rootNode_.get(), node, i);
    }
    YGNodeCalculateLayout(rootNode_.get(), YGUndefined, YGUndefined,
                          YGDirectionLTR);
    width_ = static_cast<int>(YGNodeLayoutGetWidth(rootNode_.get()));
    height_ = static_cast<int>(YGNodeLayoutGetHeight(rootNode_.get()));

    for (size_t i = 0; i < items.size(); i++) {
        auto *node = itemNodes_[i].get();
        const int left = static_cast<int>(YGNodeLayoutGetLeft(node));
        const int top = static_cast<int>(YGNodeLayoutGetTop(node));
        if (items[i].separator) {
            rows_.push_back({Rect().setPosition(left, top).setSize(
                                 itemWidth + style.text.left + style.text.right,
                                 style.separatorHeight),
                             Rect(), Rect(), true});
        } else {
            const int rowTop = top - style.text.top;
            const int rowHeight =
                itemHeight + style.text.top + style.text.bottom;
            rows_.push_back(
                {Rect()
                     .setPosition(left - style.text.left, rowTop)
                     .setSize(itemWidth + style.text.left + style.text.right,
                              rowHeight),
                 Rect()
                     .setPosition(left,
                                  rowTop + (rowHeight - style.checkHeight) / 2)
                     .setSize(style.checkWidth, style.checkHeight),
                 Rect()
                     .setPosition(left + style.checkWidth,
                                  rowTop +
                                      (rowHeight - items[i].textHeight) / 2)
                     .setSize(items[i].textWidth, items[i].textHeight),
                 false});
        }
    }
}

int CandidateMenuLayout::indexAt(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return -1;
    }
    for (size_t i = 0; i < rows_.size(); i++) {
        const auto &region = rows_[i].region;
        if (!rows_[i].separator && x >= region.left() - highlight_.left &&
            x < region.right() + highlight_.right &&
            y >= region.top() - highlight_.top &&
            y < region.bottom() + highlight_.bottom) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace fcitx::classicui
