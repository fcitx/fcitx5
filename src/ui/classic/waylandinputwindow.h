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
#ifndef _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_

#include "inputwindow.h"
#include "zwp_input_panel_surface_v1.h"

namespace fcitx {
namespace classicui {

class WaylandUI;
class WaylandWindow;

class WaylandInputWindow : public InputWindow {
public:
    WaylandInputWindow(WaylandUI *ui);

    void initPanel();
    void resetPanel();
    void update(InputContext *ic);

private:
    WaylandUI *ui_;
    std::unique_ptr<wayland::ZwpInputPanelSurfaceV1> panelSurface_;
    std::unique_ptr<WaylandWindow> window_;
    TrackableObjectReference<InputContext> repaintIC_;
};

} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_
