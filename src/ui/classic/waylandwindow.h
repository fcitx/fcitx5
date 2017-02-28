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
#ifndef _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_

#include "waylandui.h"
#include "window.h"
#include "wl_surface.h"
#include <cairo/cairo.h>

namespace fcitx {
namespace classicui {

class WaylandWindow : public Window {
public:
    WaylandWindow(WaylandUI *ui, UserInterfaceComponent type);
    ~WaylandWindow();

    virtual void createWindow();
    virtual void destroyWindow();

protected:
    WaylandUI *ui_;
    std::unique_ptr<wayland::WlSurface> surface_;
};
}
}

#endif // _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_
