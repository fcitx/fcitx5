/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_UI_CLASSIC_CANDIDATEMENUPLACEMENT_H_
#define _FCITX_UI_CLASSIC_CANDIDATEMENUPLACEMENT_H_

#include <algorithm>
#include <optional>
#include "fcitx-utils/rect.h"

namespace fcitx::classicui {

// The input rectangle is given in input-popup-local coordinates by the v2
// compositor. It reveals which side of the caret the compositor chose for
// the panel without requiring the panel's unavailable screen position.
inline Rect candidateMenuPosition(const Rect &candidate, int panelWidth,
                                  int panelHeight, int menuWidth,
                                  int menuHeight,
                                  const std::optional<Rect> &inputRect) {
    int x =
        std::clamp(candidate.left(), 0, std::max(0, panelWidth - menuWidth));
    if (inputRect && inputRect->left() >= panelWidth) {
        x = std::min(x, panelWidth - menuWidth);
    }

    // Default to below the entire candidate panel, including on legacy
    // input panels, so the menu does not cover the text being entered. If
    // the v2 compositor placed the panel above the text, open above it.
    const int y = inputRect && inputRect->top() >= panelHeight ? -menuHeight
                                                               : panelHeight;
    return Rect().setPosition(x, y).setSize(menuWidth, menuHeight);
}

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_CANDIDATEMENUPLACEMENT_H_
