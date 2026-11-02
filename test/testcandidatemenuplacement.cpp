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
    const Rect clickedPoint(155, 180, 156, 181);
    const auto menu = candidateMenuPosition(clickedPoint, 320, 200, 150, 170);
    FCITX_ASSERT(candidateMenuAvailableWidth(clickedPoint.left(), 320) == 165);
    FCITX_ASSERT(candidateMenuAvailableHeight(clickedPoint.top(), 200) == 180);
    FCITX_ASSERT(menu.left() == 155);
    FCITX_ASSERT(menu.top() == 10);
    FCITX_ASSERT(menu.right() <= 320);
    FCITX_ASSERT(menu.bottom() == clickedPoint.top());

    // Flip left when the menu would cross the screen's right edge, then clamp
    // it to the screen when the full menu still fits.
    const Rect screen(0, 0, 1920, 1080);
    FCITX_ASSERT(candidateMenuPositionX(600, 1400, screen) == 0);
    FCITX_ASSERT(candidateMenuPositionY(400, 700, screen) == 0);
    FCITX_ASSERT(candidateMenuPositionY(1000, 700, screen) == 300);

    return 0;
}
