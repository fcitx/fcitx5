/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_

#include <memory>
#include <wayland-util.h>
#include "fcitx-utils/trackableobject.h"
#include "fcitx/inputcontext.h"
#include "inputwindow.h"
#include "org_kde_kwin_blur.h"
#include "org_kde_kwin_blur_manager.h"
#include "zwp_input_panel_surface_v1.h"
#include "zwp_input_popup_surface_v2.h"

namespace fcitx::classicui {

class WaylandUI;
class WaylandWindow;

class WaylandInputWindow : public InputWindow {
public:
    WaylandInputWindow(WaylandUI *ui);

    void initPanel();
    void resetPanel();
    void update(InputContext *ic);
    void repaint();
    void setBlurManager(std::shared_ptr<wayland::OrgKdeKwinBlurManager> blur);
    void updateScale();

private:
    void updateBlur();

    WaylandUI *ui_;
    wl_fixed_t scroll_ = 0;
    std::unique_ptr<wayland::ZwpInputPanelSurfaceV1> panelSurface_;
    TrackableObjectReference<InputContext> v2IC_;
    std::unique_ptr<wayland::ZwpInputPopupSurfaceV2> panelSurfaceV2_;
    std::unique_ptr<WaylandWindow> window_;
    TrackableObjectReference<InputContext> repaintIC_;
    std::shared_ptr<wayland::OrgKdeKwinBlurManager> blurManager_;
    std::unique_ptr<wayland::OrgKdeKwinBlur> blur_;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_
