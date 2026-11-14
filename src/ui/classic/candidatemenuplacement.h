/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_UI_CLASSIC_CANDIDATEMENUPLACEMENT_H_
#define _FCITX_UI_CLASSIC_CANDIDATEMENUPLACEMENT_H_

#include <algorithm>
#include "fcitx-utils/rect.h"

namespace fcitx::classicui {

inline int candidateMenuPositionX(const Rect &anchor, int menuWidth,
                                  const Rect &screen) {
    int x = anchor.left();
    if (x + menuWidth > screen.right()) {
        x = anchor.right() - menuWidth;
    }
    return std::clamp(x, screen.left(),
                      std::max(screen.left(), screen.right() - menuWidth));
}

inline int candidateMenuPositionY(const Rect &anchor, int menuHeight,
                                  const Rect &screen) {
    int y = anchor.bottom();
    if (y + menuHeight > screen.bottom()) {
        y = anchor.top() - menuHeight;
    }
    return std::clamp(y, screen.top(),
                      std::max(screen.top(), screen.bottom() - menuHeight));
}

// Keep the menu aligned to the candidate region. Flip within the visible input
// panel only when the flipped side fits without crossing the top or left edge.
inline Rect candidateMenuPosition(const Rect &anchor, int panelWidth,
                                  int panelHeight, int menuWidth,
                                  int menuHeight) {
    const int x =
        (anchor.left() + menuWidth > panelWidth && anchor.right() >= menuWidth)
            ? anchor.right() - menuWidth
            : anchor.left();
    const int y = (anchor.bottom() + menuHeight > panelHeight &&
                   anchor.top() >= menuHeight)
                      ? anchor.top() - menuHeight
                      : anchor.bottom();
    return Rect().setPosition(x, y).setSize(menuWidth, menuHeight);
}

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_CANDIDATEMENUPLACEMENT_H_
