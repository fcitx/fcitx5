/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "fcitx-utils/log.h"
#include "candidatemenulayout.h"

using namespace fcitx::classicui;

int main() {
    CandidateMenuLayout layout;
    CandidateMenuLayout::Style style{
        .content = {.left = 5, .right = 7, .top = 11, .bottom = 13},
        .text = {.left = 3, .right = 4, .top = 2, .bottom = 3},
        .highlight = {.left = 3, .right = 4, .top = 2, .bottom = 3},
        .spacing = 6,
        .checkWidth = 8,
        .checkHeight = 8,
        .separatorHeight = 2,
    };
    layout.update(style, {
                             {.textWidth = 20, .textHeight = 10},
                             {.separator = true},
                             {.textWidth = 12, .textHeight = 14},
                         });

    FCITX_ASSERT(layout.width() == 47);
    FCITX_ASSERT(layout.height() == 76);
    FCITX_ASSERT(layout.row(0).text.left() == layout.row(2).text.left());
    FCITX_ASSERT(layout.row(0).check.left() < layout.row(0).text.left());
    FCITX_ASSERT(layout.indexAt(6, 12) == 0);
    FCITX_ASSERT(layout.indexAt(6, 37) == -1);
    FCITX_ASSERT(layout.indexAt(6, 45) == 2);

    // Matching TextMargin and Highlight/Margin (as in the default theme)
    // produces adjacent, non-overlapping highlight regions inside
    // ContentMargin.
    style = {
        .content = {.left = 2, .right = 2, .top = 2, .bottom = 2},
        .text = {.left = 5, .right = 5, .top = 5, .bottom = 5},
        .highlight = {.left = 5, .right = 5, .top = 5, .bottom = 5},
    };
    layout.update(style, {
                             {.textWidth = 40, .textHeight = 18},
                             {.textWidth = 40, .textHeight = 18},
                         });
    FCITX_ASSERT(layout.row(0).region.left() == 2);
    FCITX_ASSERT(layout.row(0).region.right() == layout.width() - 2);
    FCITX_ASSERT(layout.row(0).region.top() == 2);
    FCITX_ASSERT(layout.row(0).region.bottom() == layout.row(1).region.top());
    FCITX_ASSERT(layout.row(1).region.bottom() == layout.height() - 2);

    style = {};
    layout.update(style, {{.textWidth = 12, .textHeight = 10}});

    FCITX_ASSERT(layout.width() == 12);
    FCITX_ASSERT(layout.height() == 10);
    FCITX_ASSERT(layout.indexAt(1, 1) == 0);
    FCITX_ASSERT(layout.indexAt(6, 45) == -1);

    layout.update(style, {
                             {.textWidth = 12, .textHeight = 10},
                             {.textWidth = 12, .textHeight = 10},
                         });
    FCITX_ASSERT(layout.indexAt(1, 10) == 1);
    FCITX_ASSERT(layout.indexAt(12, 1) == -1);

    style = {
        .content = {.left = 3, .right = 4, .top = 2, .bottom = 3},
        .highlight = {.left = 4, .right = 5, .top = 3, .bottom = 4},
    };
    layout.update(style, {
                             {.textWidth = 12, .textHeight = 10},
                             {.textWidth = 12, .textHeight = 10},
                         });
    FCITX_ASSERT(layout.indexAt(0, 0) == 0);
    FCITX_ASSERT(layout.indexAt(16, 20) == 1);
    FCITX_ASSERT(layout.indexAt(20, 20) == -1);
    FCITX_ASSERT(layout.indexAt(-1, -1) == -1);

    style = {
        .text = {.top = 1, .bottom = 5},
        .checkWidth = 8,
        .checkHeight = 8,
    };
    layout.update(style, {{.textWidth = 12, .textHeight = 10}});
    const auto &row = layout.row(0);
    FCITX_ASSERT(row.check.top() ==
                 row.region.top() + (row.region.height() - 8) / 2);
    FCITX_ASSERT(row.text.top() ==
                 row.region.top() + (row.region.height() - 10) / 2);
    return 0;
}
