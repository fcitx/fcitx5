/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_

#include "inputwindow.h"
#include "zwp_input_panel_surface_v1.h"
#include "zwp_input_popup_surface_v2.h"

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
    void repaint();

private:
    WaylandUI *ui_;
    wl_fixed_t scroll_ = 0;
    std::unique_ptr<wayland::ZwpInputPanelSurfaceV1> panelSurface_;
    TrackableObjectReference<InputContext> v2IC_;
    std::unique_ptr<wayland::ZwpInputPopupSurfaceV2> panelSurfaceV2_;
    std::unique_ptr<WaylandWindow> window_;
    TrackableObjectReference<InputContext> repaintIC_;
};

} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_
