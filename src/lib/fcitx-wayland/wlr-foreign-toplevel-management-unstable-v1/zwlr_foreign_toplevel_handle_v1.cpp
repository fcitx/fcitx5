#include "zwlr_foreign_toplevel_handle_v1.h"
#include <cassert>
#include "wl_output.h"
#include "wl_seat.h"
#include "wl_surface.h"
namespace fcitx::wayland {
const struct zwlr_foreign_toplevel_handle_v1_listener
    ZwlrForeignToplevelHandleV1::listener = {
        [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
           const char *title) {
            auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
            assert(*obj == wldata);
            { return obj->title()(title); }
        },
        [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
           const char *appId) {
            auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
            assert(*obj == wldata);
            { return obj->appId()(appId); }
        },
        [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
           wl_output *output) {
            auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
            assert(*obj == wldata);
            {
                if (!output) {
                    return;
                }
                auto *output_ =
                    static_cast<WlOutput *>(wl_output_get_user_data(output));
                return obj->outputEnter()(output_);
            }
        },
        [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
           wl_output *output) {
            auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
            assert(*obj == wldata);
            {
                if (!output) {
                    return;
                }
                auto *output_ =
                    static_cast<WlOutput *>(wl_output_get_user_data(output));
                return obj->outputLeave()(output_);
            }
        },
        [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
           wl_array *state) {
            auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
            assert(*obj == wldata);
            { return obj->state()(state); }
        },
        [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata) {
            auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
            assert(*obj == wldata);
            { return obj->done()(); }
        },
        [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata) {
            auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
            assert(*obj == wldata);
            { return obj->closed()(); }
        },
        [](void *data, zwlr_foreign_toplevel_handle_v1 *wldata,
           zwlr_foreign_toplevel_handle_v1 *parent) {
            auto *obj = static_cast<ZwlrForeignToplevelHandleV1 *>(data);
            assert(*obj == wldata);
            {
                auto *parent_ =
                    parent ? static_cast<ZwlrForeignToplevelHandleV1 *>(
                                 zwlr_foreign_toplevel_handle_v1_get_user_data(
                                     parent))
                           : nullptr;
                return obj->parent()(parent_);
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
    auto version = zwlr_foreign_toplevel_handle_v1_get_version(data);
    if (version >= 1) {
        return zwlr_foreign_toplevel_handle_v1_destroy(data);
    }
}
void ZwlrForeignToplevelHandleV1::setMaximized() {
    return zwlr_foreign_toplevel_handle_v1_set_maximized(*this);
}
void ZwlrForeignToplevelHandleV1::unsetMaximized() {
    return zwlr_foreign_toplevel_handle_v1_unset_maximized(*this);
}
void ZwlrForeignToplevelHandleV1::setMinimized() {
    return zwlr_foreign_toplevel_handle_v1_set_minimized(*this);
}
void ZwlrForeignToplevelHandleV1::unsetMinimized() {
    return zwlr_foreign_toplevel_handle_v1_unset_minimized(*this);
}
void ZwlrForeignToplevelHandleV1::activate(WlSeat *seat) {
    return zwlr_foreign_toplevel_handle_v1_activate(*this, rawPointer(seat));
}
void ZwlrForeignToplevelHandleV1::close() {
    return zwlr_foreign_toplevel_handle_v1_close(*this);
}
void ZwlrForeignToplevelHandleV1::setRectangle(WlSurface *surface, int32_t x,
                                               int32_t y, int32_t width,
                                               int32_t height) {
    return zwlr_foreign_toplevel_handle_v1_set_rectangle(
        *this, rawPointer(surface), x, y, width, height);
}
void ZwlrForeignToplevelHandleV1::setFullscreen(WlOutput *output) {
    return zwlr_foreign_toplevel_handle_v1_set_fullscreen(*this,
                                                          rawPointer(output));
}
void ZwlrForeignToplevelHandleV1::unsetFullscreen() {
    return zwlr_foreign_toplevel_handle_v1_unset_fullscreen(*this);
}
} // namespace fcitx::wayland
