/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_UI_CLASSIC_XCBTRAYWINDOW_H_
#define _FCITX_UI_CLASSIC_XCBTRAYWINDOW_H_

#include "xcbwindow.h"

namespace fcitx {
namespace classicui {

class XCBTrayWindow : public XCBWindow {
public:
    XCBTrayWindow(XCBUI *ui);

    bool filterEvent(xcb_generic_event_t *event) override;

private:
    void initTray();
    void findDock();
    void sendTrayOpcode(long message, long data1, long data2, long data3);
    void refreshDockWindow();
    xcb_visualid_t trayVisual();
    void paint(cairo_t *cr);

    xcb_window_t dockWindow_ = XCB_WINDOW_NONE;
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
        dockCallback_;

    xcb_atom_t atoms_[5];
};
}
}

#endif // _FCITX_UI_CLASSIC_XCBTRAYWINDOW_H_
