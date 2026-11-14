/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "fcitx-utils/log.h"
#include "fcitx-utils/rect.h"
#include "candidatemenuplacement.h"

using namespace fcitx;
using namespace fcitx::classicui;

int main() {
    // Open below a horizontal candidate bar (including when preedit sits above
    // the candidate row) rather than flipping into negative coordinates.
    const Rect horizontalCandidate(74, 28, 146, 56);
    auto menu = candidateMenuPosition(horizontalCandidate, 515, 58, 110, 40);
    FCITX_ASSERT(menu.left() == horizontalCandidate.left());
    FCITX_ASSERT(menu.top() == horizontalCandidate.bottom());

    // Flip to right-align with the candidate when crossing the panel's right
    // edge if doing so fits within the panel.
    const Rect rightCandidate(430, 2, 500, 32);
    menu = candidateMenuPosition(rightCandidate, 515, 34, 110, 40);
    FCITX_ASSERT(menu.right() == rightCandidate.right());
    FCITX_ASSERT(menu.top() == rightCandidate.bottom());

    // Flip above a lower candidate in a vertical candidate list when the menu
    // fits above the candidate inside the panel.
    const Rect lowerCandidate(155, 175, 235, 198);
    menu = candidateMenuPosition(lowerCandidate, 320, 200, 150, 170);
    FCITX_ASSERT(menu.left() == lowerCandidate.left());
    FCITX_ASSERT(menu.top() == 5);
    FCITX_ASSERT(menu.right() <= 320);
    FCITX_ASSERT(menu.bottom() == lowerCandidate.top());

    // Flip left/up when the menu would cross the screen's edges, then clamp
    // it to the screen when the full menu still fits.
    const Rect screen(0, 0, 1920, 1080);
    FCITX_ASSERT(
        candidateMenuPositionX(Rect(600, 400, 700, 430), 1400, screen) == 0);
    FCITX_ASSERT(candidateMenuPositionX(Rect(1800, 400, 1900, 430), 200,
                                        screen) == 1700);
    FCITX_ASSERT(
        candidateMenuPositionY(Rect(500, 400, 600, 430), 700, screen) == 0);
    FCITX_ASSERT(
        candidateMenuPositionY(Rect(500, 1000, 600, 1030), 700, screen) == 300);

    return 0;
}
