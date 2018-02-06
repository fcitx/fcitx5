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
#ifndef _FCITX_UI_CLASSIC_XCBINPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_XCBINPUTWINDOW_H_

#include "inputwindow.h"
#include "xcbwindow.h"

namespace fcitx {
namespace classicui {

class XCBInputWindow : public XCBWindow, protected InputWindow {
public:
    XCBInputWindow(XCBUI *ui);

    void postCreateWindow() override;
    void update(InputContext *c);
    void updatePosition(InputContext *c);

    bool filterEvent(xcb_generic_event_t *event) override;

    void updateDPI(InputContext *c);
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_XCBINPUTWINDOW_H_
