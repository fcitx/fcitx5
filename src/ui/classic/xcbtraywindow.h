//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_UI_CLASSIC_XCBTRAYWINDOW_H_
#define _FCITX_UI_CLASSIC_XCBTRAYWINDOW_H_

#include "fcitx/menu.h"
#include "xcbmenu.h"
#include "xcbwindow.h"

namespace fcitx {
namespace classicui {

class XCBTrayWindow : public XCBWindow {
public:
    XCBTrayWindow(XCBUI *ui);
    void initTray();

    bool filterEvent(xcb_generic_event_t *event) override;
    void resume();
    void suspend();
    void update();
    void postCreateWindow() override;

    void updateMenu();
    void updateGroupMenu();
    void updateInputMethodMenu();

private:
    void findDock();
    void sendTrayOpcode(long message, long data1, long data2, long data3);
    void refreshDockWindow();
    xcb_visualid_t trayVisual();
    void paint(cairo_t *cr);

    xcb_window_t dockWindow_ = XCB_WINDOW_NONE;
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
        dockCallback_;

    xcb_atom_t atoms_[5];

    MenuPool menuPool_;

    Menu menu_;
    SimpleAction inputMethodAction_;
    SimpleAction groupAction_;
    SimpleAction separatorActions_[3];
    SimpleAction configureCurrentAction_;
    SimpleAction configureAction_;
    SimpleAction restartAction_;
    SimpleAction exitAction_;

#if 0
    SimpleAction testAction1_;
    SimpleAction testAction2_;
    Menu testMenu1_;
    Menu testMenu2_;
    SimpleAction testSubAction1_;
    SimpleAction testSubAction2_;
#endif

    Menu groupMenu_;
    std::list<SimpleAction> groupActions_;
    Menu inputMethodMenu_;
    std::list<SimpleAction> inputMethodActions_;
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_XCBTRAYWINDOW_H_
