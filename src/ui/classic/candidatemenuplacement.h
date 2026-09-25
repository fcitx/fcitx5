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
                                  int menuWidth, int menuHeight,
                                  const std::optional<Rect> &inputRect) {
    int x =
        std::clamp(candidate.left(), 0, std::max(0, panelWidth - menuWidth));
    if (inputRect && inputRect->left() >= panelWidth) {
        x = std::min(x, panelWidth - menuWidth);
    }

    // Legacy input panels do not provide the input rectangle; keep the
    // existing above-panel placement for them.
    const int y = inputRect && inputRect->bottom() <= 0
                      ? candidate.bottom()
                      : candidate.top() - menuHeight;
    return Rect().setPosition(x, y).setSize(menuWidth, menuHeight);
}

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_CANDIDATEMENUPLACEMENT_H_
