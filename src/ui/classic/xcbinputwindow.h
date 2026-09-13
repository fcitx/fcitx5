/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_XCBINPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_XCBINPUTWINDOW_H_

#include <list>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include "fcitx-utils/rect.h"
#include "fcitx/action.h"
#include "fcitx/inputcontext.h"
#include "fcitx/menu.h"
#include "inputwindow.h"
#include "xcbmenu.h"
#include "xcbui.h"
#include "xcbwindow.h"

namespace fcitx::classicui {

class XCBInputWindow : public XCBWindow, protected InputWindow {
public:
    XCBInputWindow(XCBUI *ui);

    void postCreateWindow() override;
    void update(InputContext *inputContext);
    void updatePosition(InputContext *inputContext);

    bool filterEvent(xcb_generic_event_t *event) override;

    void updateDPI(InputContext *inputContext);

private:
    /// Build and show the candidate action menu at the pointer position.
    bool showCandidateMenu(int x, int y, int rootX, int rootY);
    /// Hide the candidate action menu and unregister its temporary actions.
    void clearCandidateMenu();
    void repaint();
    const Rect *getClosestScreen(const Rect &cursorRect) const;
    int calculatePositionX(const Rect &cursorRect,
                           const Rect *closestScreen) const;
    int calculatePositionY(const Rect &cursorRect,
                           const Rect *closestScreen) const;

    xcb_atom_t atomBlur_;

    MenuPool candidateMenuPool_;
    Menu candidateMenu_;
    std::list<SimpleAction> candidateActions_;
    XCBMenu *candidateMenuWindow_ = nullptr;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_XCBINPUTWINDOW_H_
