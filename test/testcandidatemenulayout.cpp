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
    CandidateMenuLayout::Style style;
    style.content = {5, 7, 11, 13};
    style.text = {3, 4, 2, 3};
    style.spacing = 6;
    style.checkWidth = 8;
    style.checkHeight = 8;
    style.separatorHeight = 2;
    layout.update(style, {{20, 10, false}, {0, 0, true}, {12, 14, false}});

    FCITX_ASSERT(layout.width() == 47);
    FCITX_ASSERT(layout.height() == 76);
    FCITX_ASSERT(layout.row(0).text.left() == layout.row(2).text.left());
    FCITX_ASSERT(layout.row(0).check.left() < layout.row(0).text.left());
    FCITX_ASSERT(layout.indexAt(6, 12) == 0);
    FCITX_ASSERT(layout.indexAt(6, 37) == -1);
    FCITX_ASSERT(layout.indexAt(6, 45) == 2);

    style.content = {0, 0, 0, 0};
    style.text = {0, 0, 0, 0};
    style.spacing = 0;
    style.checkWidth = 0;
    style.checkHeight = 0;
    layout.update(style, {{12, 10, false}});

    FCITX_ASSERT(layout.width() == 12);
    FCITX_ASSERT(layout.height() == 10);
    FCITX_ASSERT(layout.indexAt(1, 1) == 0);
    FCITX_ASSERT(layout.indexAt(6, 45) == -1);

    layout.update(style, {{12, 10, false}, {12, 10, false}});
    FCITX_ASSERT(layout.indexAt(1, 10) == 1);
    FCITX_ASSERT(layout.indexAt(12, 1) == -1);

    style.content = {3, 4, 2, 3};
    style.highlight = {4, 5, 3, 4};
    layout.update(style, {{12, 10, false}, {12, 10, false}});
    FCITX_ASSERT(layout.indexAt(0, 0) == 0);
    FCITX_ASSERT(layout.indexAt(16, 20) == 1);
    FCITX_ASSERT(layout.indexAt(20, 20) == -1);
    FCITX_ASSERT(layout.indexAt(-1, -1) == -1);

    style.content = {0, 0, 0, 0};
    style.text = {0, 0, 1, 5};
    style.highlight = {};
    style.checkWidth = 8;
    style.checkHeight = 8;
    layout.update(style, {{12, 10, false}});
    const auto &row = layout.row(0);
    FCITX_ASSERT(row.check.top() ==
                 row.region.top() + (row.region.height() - 8) / 2);
    FCITX_ASSERT(row.text.top() ==
                 row.region.top() + (row.region.height() - 10) / 2);
    return 0;
}
