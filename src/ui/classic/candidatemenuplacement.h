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

inline int candidateMenuPositionX(int cursorX, int menuWidth,
                                  const Rect &screen) {
    int x = cursorX + 1;
    if (x + menuWidth > screen.right()) {
        x = cursorX - menuWidth;
    }
    return std::clamp(x, screen.left(),
                      std::max(screen.left(), screen.right() - menuWidth));
}

inline int candidateMenuPositionY(int cursorY, int menuHeight,
                                  const Rect &screen) {
    int y = cursorY;
    if (y + menuHeight > screen.bottom()) {
        y = cursorY - menuHeight;
    }
    return std::clamp(y, screen.top(),
                      std::max(screen.top(), screen.bottom() - menuHeight));
}

inline int candidateMenuAvailableWidth(int x, int panelWidth) {
    return std::max(x, panelWidth - x);
}

inline int candidateMenuAvailableHeight(int y, int panelHeight) {
    return std::max(y, panelHeight - y);
}

// Keep the menu inside the visible input panel. Wayland does not expose the
// panel's absolute screen position, so this is the available placement area.
inline Rect candidateMenuPosition(const Rect &anchor, int panelWidth,
                                  int panelHeight, int menuWidth,
                                  int menuHeight) {
    const int x = anchor.left() > panelWidth - anchor.left()
                      ? anchor.left() - menuWidth
                      : anchor.left();
    const int y = anchor.top() > panelHeight - anchor.top()
                      ? anchor.top() - menuHeight
                      : anchor.top();
    return Rect().setPosition(x, y).setSize(menuWidth, menuHeight);
}

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_CANDIDATEMENUPLACEMENT_H_
