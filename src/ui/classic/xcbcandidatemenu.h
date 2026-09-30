/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_UI_CLASSIC_XCBCANDIDATEMENU_H_
#define _FCITX_UI_CLASSIC_XCBCANDIDATEMENU_H_

#include <vector>
#include <xcb/xcb.h>
#include "candidatemenu.h"
#include "xcbwindow.h"

namespace fcitx::classicui {

class XCBCandidateMenu : public XCBWindow {
public:
    explicit XCBCandidateMenu(XCBUI *ui);
    bool show(InputContext *inputContext, const std::vector<Rect> &regions,
              int x, int y, int rootX, int rootY);
    void hide();
    bool visible() const { return visible_; }
    bool filterEvent(xcb_generic_event_t *event) override;
    void postCreateWindow() override;

private:
    void repaint();
    void updateBlur();

    CandidateMenu menu_;
    xcb_atom_t atomBlur_;
    bool visible_ = false;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_XCBCANDIDATEMENU_H_
