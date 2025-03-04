#include "zwlr_foreign_toplevel_manager_v1.h"
#include <cassert>
#include "wayland-wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "zwlr_foreign_toplevel_handle_v1.h"

namespace fcitx::wayland {
const struct zwlr_foreign_toplevel_manager_v1_listener
    ZwlrForeignToplevelManagerV1::listener = {
        .toplevel =
            [](void *data, zwlr_foreign_toplevel_manager_v1 *wldata,
               zwlr_foreign_toplevel_handle_v1 *toplevel) {
                auto *obj = static_cast<ZwlrForeignToplevelManagerV1 *>(data);
                assert(*obj == wldata);
                {
                    auto *toplevel_ = new ZwlrForeignToplevelHandleV1(toplevel);
                    obj->toplevel()(toplevel_);
                }
            },
        .finished =
            [](void *data, zwlr_foreign_toplevel_manager_v1 *wldata) {
                auto *obj = static_cast<ZwlrForeignToplevelManagerV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->finished()();
                }
            },
};

ZwlrForeignToplevelManagerV1::ZwlrForeignToplevelManagerV1(
    zwlr_foreign_toplevel_manager_v1 *data)
    : version_(zwlr_foreign_toplevel_manager_v1_get_version(data)),
      data_(data) {
    zwlr_foreign_toplevel_manager_v1_set_user_data(*this, this);
    zwlr_foreign_toplevel_manager_v1_add_listener(
        *this, &ZwlrForeignToplevelManagerV1::listener, this);
}

void ZwlrForeignToplevelManagerV1::destructor(
    zwlr_foreign_toplevel_manager_v1 *data) {
    zwlr_foreign_toplevel_manager_v1_destroy(data);
}
void ZwlrForeignToplevelManagerV1::stop() {
    zwlr_foreign_toplevel_manager_v1_stop(*this);
}

} // namespace fcitx::wayland
