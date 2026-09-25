/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "fcitx-utils/log.h"
#include "candidatemenuplacement.h"

using namespace fcitx;
using namespace fcitx::classicui;

int main() {
    const Rect candidate(40, 8, 100, 36);

    // Legacy input panels have no compositor placement hint. The menu should
    // open below the candidate bar instead of covering the text being typed.
    auto menu =
        candidateMenuPosition(candidate, 160, 40, 110, 120, std::nullopt);
    FCITX_ASSERT(menu.top() == 40);

    // The compositor has placed the input area above this popup. There is
    // room below the candidate bar, regardless of the bar's own height.
    const Rect inputAbove(40, -22, 56, -2);
    menu = candidateMenuPosition(candidate, 160, 40, 110, 120, inputAbove);
    FCITX_ASSERT(menu.top() == 40);

    // When KWin flips the input popup above the caret, use that same side.
    const Rect inputBelow(40, 50, 56, 70);
    menu = candidateMenuPosition(candidate, 160, 40, 110, 120, inputBelow);
    FCITX_ASSERT(menu.bottom() == 0);

    // A menu opened from a lower row of a vertical candidate list must not
    // cover the other candidates when the compositor places it above.
    const Rect lowerCandidate(40, 60, 100, 84);
    const Rect inputFarBelow(40, 110, 56, 130);
    menu = candidateMenuPosition(lowerCandidate, 160, 100, 110, 120,
                                 inputFarBelow);
    FCITX_ASSERT(menu.bottom() == 0);

    // The text area to the right of the panel indicates that the compositor
    // shifted the panel left at the screen edge. Keep the menu's right edge
    // within the panel even if the menu is wider than the panel.
    const Rect inputRight(165, 8, 181, 28);
    menu = candidateMenuPosition(candidate, 160, 40, 220, 80, inputRight);
    FCITX_ASSERT(menu.right() == 160);

    return 0;
}
