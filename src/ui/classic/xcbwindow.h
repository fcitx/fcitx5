/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#ifndef _FCITX_UI_CLASSIC_XCBWINDOW_H_
#define _FCITX_UI_CLASSIC_XCBWINDOW_H_

#include "window.h"
#include "xcbui.h"
#include <cairo/cairo.h>
#include <xcb/xcb.h>

namespace fcitx {
namespace classicui {

class XCBWindow : public Window {
public:
    XCBWindow(XCBUI *ui);
    ~XCBWindow();

    void createWindow();
    void destroyWindow();
    void resize(unsigned int width, unsigned int height);

    virtual bool filterEvent(xcb_generic_event_t *event) = 0;

private:
    XCBUI *ui_;
    xcb_window_t wid_ = 0;
    std::unique_ptr<HandlerTableEntry<XCBEventFilter>> eventFilter_;
    cairo_surface_t *surface_ = nullptr;
};
}
}

#endif // _FCITX_UI_CLASSIC_XCBWINDOW_H_
