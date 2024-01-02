/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDCURSOR_H_
#define _FCITX_UI_CLASSIC_WAYLANDCURSOR_H_

#include <memory>
#include <optional>
#include <wayland-cursor.h>
#include "fcitx-utils/event.h"
#include "fcitx-utils/signals.h"
#include "display.h"
#include "wl_callback.h"
#include "wl_surface.h"
#include "wp_cursor_shape_device_v1.h"
#include "wp_cursor_shape_manager_v1.h"

namespace fcitx::classicui {

class WaylandPointer;

class WaylandCursor {
public:
    WaylandCursor(WaylandPointer *pointer);

    // Size and hotspot are in surface coordinates
    void update();

private:
    int32_t scale();
    void timerCallback();
    void frameCallback();
    void maybeUpdate();
    void setupCursorShape();
    wayland::WlSurface *getOrCreateSurface();
    WaylandPointer *pointer_;
    uint64_t animationStart_;

    // We keep a reference to the
    std::shared_ptr<wl_cursor_theme> theme_;
    std::unique_ptr<wayland::WlSurface> surface_;
    std::unique_ptr<wayland::WlCallback> callback_;
    std::unique_ptr<EventSourceTime> time_;
    bool timerCalled_ = false;
    bool frameCalled_ = false;
    std::optional<int32_t> outputScale_;
    ScopedConnection cursorShapeGlobalConn_;
    std::unique_ptr<wayland::WpCursorShapeDeviceV1> cursorShape_;
};

} // namespace fcitx::classicui

#endif