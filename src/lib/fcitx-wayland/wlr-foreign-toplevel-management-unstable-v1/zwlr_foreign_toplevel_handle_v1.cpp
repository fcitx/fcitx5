#include "zwlr_foreign_toplevel_handle_v1.h"
#include <cassert>
#include "wayland-wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "wl_output.h"
#include "wl_seat.h"
#include "wl_surface.h"

namespace fcitx::wayland {
const struct zwlr_foreign_toplevel_handle_v1_listener
    ZwlrForeignToplevelHandleV1::listener = {
        .title =
            [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
               const char *title) {
                auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->title()(title);
                }
            },
        .app_id =
            [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
               const char *appId) {
                auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->appId()(appId);
                }
            },
        .output_enter =
            [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
               wl_output *output) {
                auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    if (!output) {
                        return;
                    }
                    auto *output_ = static_cast<WlOutput *>(
                        wl_output_get_user_data(output));
                    obj->outputEnter()(output_);
                }
            },
        .output_leave =
            [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
               wl_output *output) {
                auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    if (!output) {
                        return;
                    }
                    auto *output_ = static_cast<WlOutput *>(
                        wl_output_get_user_data(output));
                    obj->outputLeave()(output_);
                }
            },
        .state =
            [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
               wl_array *state) {
                auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->state()(state);
                }
            },
        .done =
            [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata) {
                auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->done()();
                }
            },
        .closed =
            [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata) {
                auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->closed()();
                }
            },
        .parent =
            [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
               zwlr_foreign_toplevel_handle_v1 *parent) {
                auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    auto *parent_ =
                        parent
                            ? static_cast<ZwlrForeignToplevelHandleV1 *>(
                                  zwlr_foreign_toplevel_handle_v1_get_user_data(
                                      parent))
                            : nullptr;
                    obj->parent()(parent_);
                }
            },
};

ZwlrForeignToplevelHandleV1::ZwlrForeignToplevelHandleV1(
    zwlr_foreign_toplevel_handle_v1 *data)
    : version_(zwlr_foreign_toplevel_handle_v1_get_version(data)), data_(data) {
    zwlr_foreign_toplevel_handle_v1_set_user_data(*this, this);
    zwlr_foreign_toplevel_handle_v1_add_listener(
        *this, &ZwlrForeignToplevelHandleV1::listener, this);
}

void ZwlrForeignToplevelHandleV1::destructor(
    zwlr_foreign_toplevel_handle_v1 *data) {
    const auto version = zwlr_foreign_toplevel_handle_v1_get_version(data);
    if (version >= 1) {
        zwlr_foreign_toplevel_handle_v1_destroy(data);
        return;
    }
}
void ZwlrForeignToplevelHandleV1::setMaximized() {
    zwlr_foreign_toplevel_handle_v1_set_maximized(*this);
}
void ZwlrForeignToplevelHandleV1::unsetMaximized() {
    zwlr_foreign_toplevel_handle_v1_unset_maximized(*this);
}
void ZwlrForeignToplevelHandleV1::setMinimized() {
    zwlr_foreign_toplevel_handle_v1_set_minimized(*this);
}
void ZwlrForeignToplevelHandleV1::unsetMinimized() {
    zwlr_foreign_toplevel_handle_v1_unset_minimized(*this);
}
void ZwlrForeignToplevelHandleV1::activate(WlSeat *seat) {
    zwlr_foreign_toplevel_handle_v1_activate(*this, rawPointer(seat));
}
void ZwlrForeignToplevelHandleV1::close() {
    zwlr_foreign_toplevel_handle_v1_close(*this);
}
void ZwlrForeignToplevelHandleV1::setRectangle(WlSurface *surface, int32_t x,
                                               int32_t y, int32_t width,
                                               int32_t height) {
    zwlr_foreign_toplevel_handle_v1_set_rectangle(*this, rawPointer(surface), x,
                                                  y, width, height);
}
void ZwlrForeignToplevelHandleV1::setFullscreen(WlOutput *output) {
    zwlr_foreign_toplevel_handle_v1_set_fullscreen(*this, rawPointer(output));
}
void ZwlrForeignToplevelHandleV1::unsetFullscreen() {
    zwlr_foreign_toplevel_handle_v1_unset_fullscreen(*this);
}
} // namespace fcitx::wayland
