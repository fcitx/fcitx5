/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
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

    void render() override;

private:
    void findDock();
    void sendTrayOpcode(long message, long data1, long data2, long data3);
    void refreshDockWindow();
    xcb_visualid_t trayVisual();
    bool trayOrientation();
    void paint(cairo_t *cr);
    void createTrayWindow();
    void resizeTrayWindow();

    xcb_window_t dockWindow_ = XCB_WINDOW_NONE;
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
        dockCallback_;

    xcb_atom_t atoms_[5];

    MenuPool menuPool_;

    Menu menu_;
    SimpleAction inputMethodAction_;
    SimpleAction groupAction_;
    SimpleAction separatorActions_[2];
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

    xcb_visualid_t trayVid_ = 0;
    int trayDepth_ = 0;

    bool isHorizontal_ = true;
    int hintWidth_ = 0;
    int hintHeight_ = 0;

    Menu groupMenu_;
    std::list<SimpleAction> groupActions_;
    Menu inputMethodMenu_;
    std::list<SimpleAction> inputMethodActions_;
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_XCBTRAYWINDOW_H_
