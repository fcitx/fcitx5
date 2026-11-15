/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_UI_CLASSIC_XCBCANDIDATEMENU_H_
#define _FCITX_UI_CLASSIC_XCBCANDIDATEMENU_H_

#include <cstddef>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include "fcitx-utils/rect.h"
#include "fcitx-utils/signals.h"
#include "candidatemenu.h"
#include "xcbui.h"
#include "xcbwindow.h"

namespace fcitx::classicui {

class XCBCandidateMenu : public XCBWindow {
public:
    explicit XCBCandidateMenu(XCBUI *ui);
    bool show(InputContext *inputContext, size_t candidateIndex,
              const Rect &anchor);
    void hide();
    void raise();
    bool visible() const { return visible_; }
    auto &hidden() { return hidden_; }
    bool filterEvent(xcb_generic_event_t *event) override;
    void postCreateWindow() override;

private:
    void repaint();
    void updateBlur();

    CandidateMenu menu_;
    Signal<void()> hidden_;
    xcb_atom_t atomBlur_;
    bool visible_ = false;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_XCBCANDIDATEMENU_H_
