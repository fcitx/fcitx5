/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_

#include <cstdint>
#include <memory>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/rect.h"
#include "fcitx-utils/signals.h"
#include "fcitx-utils/trackableobject.h"
#include "waylandui.h"
#include "window.h"
#include "wp_fractional_scale_manager_v1.h"
#include "wp_fractional_scale_v1.h"
#include "wp_viewport.h"
#include "wp_viewporter.h"

namespace fcitx::classicui {

class WaylandWindow : public Window, public TrackableObject<WaylandWindow> {
public:
    static inline constexpr unsigned int ScaleDominator = 120;
    static inline constexpr double ScaleDominatorF = ScaleDominator;
    WaylandWindow(WaylandUI *ui);
    ~WaylandWindow();

    virtual void createWindow();
    void destroyWindow();
    virtual void hide() = 0;

    unsigned int bufferScale() const {
        return viewport_ ? lastFractionalScale_
                         : lastOutputScale_ * ScaleDominator;
    }
    int32_t outputScale() const { return lastOutputScale_; }
    wl_output_transform transform() const { return transform_; }
    bool setScaleAndTransform(int32_t scale, wl_output_transform transform) {
        if (lastOutputScale_ != scale || transform_ != transform) {
            lastOutputScale_ = scale;
            transform_ = transform;
            return true;
        }
        return false;
    }
    wayland::WlSurface *surface() { return surface_.get(); }

    auto &repaint() { return repaint_; }
    auto &hover() { return hover_; }
    auto &click() { return click_; }
    auto &axis() { return axis_; }
    auto &leave() { return leave_; }

    auto &touchDown() { return touchDown_; }
    auto &touchUp() { return touchUp_; }

    void updateScale();

protected:
    WaylandUI *ui_;
    std::unique_ptr<wayland::WlSurface> surface_;
    fcitx::ScopedConnection enterConn_;
    Signal<void()> repaint_;
    Signal<void(int, int)> hover_;
    Signal<void(int, int, uint32_t, uint32_t)> click_;
    Signal<void(int, int, uint32_t, wl_fixed_t)> axis_;
    Signal<void()> leave_;

    Signal<void(int, int)> touchDown_;
    Signal<void(int, int)> touchUp_;

    Rect serverAllocation_;
    Rect allocation_;

    int32_t lastOutputScale_ = 1;
    unsigned int lastFractionalScale_ = ScaleDominator;
    wl_output_transform transform_ = WL_OUTPUT_TRANSFORM_NORMAL;

    std::shared_ptr<wayland::WpViewporter> viewporter_;
    std::shared_ptr<wayland::WpFractionalScaleManagerV1>
        fractionalScaleManager_;

    std::unique_ptr<wayland::WpViewport> viewport_;
    std::unique_ptr<wayland::WpFractionalScaleV1> fractionalScale_;
    std::unique_ptr<EventSource> repaintEvent_;

private:
    void resetFractionalScale();
    // Avoid repaint being done too much.
    // For input window, since it will re-created frequently, the initial scale
    // might be wrong. To avoid show blurry 1x on scaled monitor, we always use
    // the last value. But just "upon" window is shown on screen, more than one
    // new scale might arrive. Do not repaint upon every scale change.
    void scheduleRepaint();
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_WAYLANDWINDOW_H_
