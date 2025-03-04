/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandcursor.h"
#include <algorithm>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string>
#include <utility>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventloopinterface.h"
#include "display.h"
#include "waylandpointer.h"
#include "waylandui.h"
#include "wl_callback.h"
#include "wl_compositor.h"
#include "wl_output.h"
#include "wl_surface.h"
#include "wp_cursor_shape_manager_v1.h"

namespace fcitx::classicui {

WaylandCursor::WaylandCursor(WaylandPointer *pointer)
    : pointer_(pointer), animationStart_(now(CLOCK_MONOTONIC)) {
    auto *display = pointer->ui()->display();
    time_ = pointer->ui()->parent()->instance()->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, 0, 1000, [this](EventSourceTime *, uint64_t) {
            timerCallback();
            return true;
        });
    time_->setEnabled(false);
    display->requestGlobals<wayland::WpCursorShapeManagerV1>();
    cursorShapeGlobalConn_ = display->globalCreated().connect(
        [this](const std::string &name, const std::shared_ptr<void> &) {
            if (name == wayland::WpCursorShapeManagerV1::interface) {
                setupCursorShape();
            }
        });
    setupCursorShape();
}

void WaylandCursor::setupCursorShape() {
    auto cursorShape =
        pointer_->ui()->display()->getGlobal<wayland::WpCursorShapeManagerV1>();
    if (!cursorShape) {
        return;
    }
    cursorShape_.reset(cursorShape->getPointer(pointer_->pointer()));
}

wayland::WlSurface *WaylandCursor::getOrCreateSurface() {
    if (!surface_) {
        auto compositor =
            pointer_->ui()->display()->getGlobal<wayland::WlCompositor>();
        surface_.reset(compositor->createSurface());
        surface_->enter().connect([this](wayland::WlOutput *output) {
            const auto *info =
                pointer_->ui()->display()->outputInformation(output);
            if (!info) {
                return;
            }
            if (outputScale_ != info->scale()) {
                outputScale_ = info->scale();
                update();
            }
        });
    }
    return surface_.get();
}

int32_t WaylandCursor::scale() {
    auto outputs = pointer_->ui()->display()->getGlobals<wayland::WlOutput>();
    int32_t scale = 1;
    if (outputScale_) {
        return *outputScale_;
    }
    for (const auto &output : outputs) {
        const auto *info =
            pointer_->ui()->display()->outputInformation(output.get());
        if (!info) {
            continue;
        }
        scale = std::max(scale, info->scale());
    }
    return scale;
}

void WaylandCursor::update() {
    if (!pointer_->enterSerial()) {
        return;
    }
    if (cursorShape_) {
        cursorShape_->setShape(pointer_->enterSerial(),
                               WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);
        surface_.reset();
        return;
    }
    auto info = pointer_->ui()->cursorTheme()->loadCursorTheme(scale());
    auto *surface = getOrCreateSurface();
    if (info.theme != theme_) {
        surface->attach(nullptr, 0, 0);
        surface->commit();
        theme_ = info.theme;
    }
    auto *cursor = info.cursor;
    if (!cursor) {
        return;
    }
    uint32_t duration = 0;
    int frame = wl_cursor_frame_and_duration(
        cursor, (now(CLOCK_MONOTONIC) - animationStart_) / 1000, &duration);

    auto bufferScale = scale();

    int pixelSize = std::max(
        cursor->images[frame]->width,
        cursor->images[frame]->height); // Not all cursor themes are square
    while (bufferScale > 1 && pixelSize / bufferScale <
                                  pointer_->ui()->cursorTheme()->cursorSize()) {
        --bufferScale;
    }

    // Do not attach width/height that is not divisible by scale.
    if (cursor->images[frame]->width % bufferScale != 0 ||
        cursor->images[frame]->height % bufferScale != 0) {
        return;
    }

    pointer_->pointer()->setCursor(
        pointer_->enterSerial(), surface,
        cursor->images[frame]->hotspot_x / bufferScale,
        cursor->images[frame]->hotspot_y / bufferScale);
    surface->setBufferScale(bufferScale);

    frameCalled_ = false;
    timerCalled_ = false;
    wl_surface_attach(*surface,
                      wl_cursor_image_get_buffer(cursor->images[frame]), 0, 0);
    surface->damage(0, 0, cursor->images[frame]->width,
                    cursor->images[frame]->height);

    std::unique_ptr<wayland::WlCallback> newCallback;
    if (duration) {
        newCallback.reset(surface->frame());
        time_->setOneShot();
        time_->setNextInterval(duration * 1000ULL);
        newCallback->done().connect([this](uint32_t) { frameCallback(); });
    }

    surface->commit();
    // Ensure callback_ is the last variable being destoryed.
    callback_ = std::move(newCallback);
}

void WaylandCursor::timerCallback() {
    timerCalled_ = true;
    maybeUpdate();
}

void WaylandCursor::frameCallback() {
    frameCalled_ = true;
    maybeUpdate();
}

void WaylandCursor::maybeUpdate() {
    if (frameCalled_ && timerCalled_) {
        update();
    }
}

} // namespace fcitx::classicui
