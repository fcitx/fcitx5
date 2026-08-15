#include "ext_foreign_toplevel_list_v1.h"
#include <cassert>
#include "ext_foreign_toplevel_handle_v1.h"
#include "wayland-ext-foreign-toplevel-list-v1-client-protocol.h"

namespace fcitx::wayland {
const struct ext_foreign_toplevel_list_v1_listener
    ExtForeignToplevelListV1::listener = {
#if defined(EXT_FOREIGN_TOPLEVEL_LIST_V1_TOPLEVEL_SINCE_VERSION)
        .toplevel =
            [](void *data, ext_foreign_toplevel_list_v1 *wldata,
               ext_foreign_toplevel_handle_v1 *toplevel) {
                auto *obj = static_cast<ExtForeignToplevelListV1 *>(data);
                assert(*obj == wldata);
                {
                    auto *toplevel_ = new ExtForeignToplevelHandleV1(toplevel);
                    obj->toplevel()(toplevel_);
                }
            },
#endif
#if defined(EXT_FOREIGN_TOPLEVEL_LIST_V1_FINISHED_SINCE_VERSION)
        .finished =
            [](void *data, ext_foreign_toplevel_list_v1 *wldata) {
                auto *obj = static_cast<ExtForeignToplevelListV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->finished()();
                }
            },
#endif
};

ExtForeignToplevelListV1::ExtForeignToplevelListV1(
    ext_foreign_toplevel_list_v1 *data)
    : version_(ext_foreign_toplevel_list_v1_get_version(data)), data_(data) {
    ext_foreign_toplevel_list_v1_set_user_data(*this, this);
    ext_foreign_toplevel_list_v1_add_listener(
        *this, &ExtForeignToplevelListV1::listener, this);
}

void ExtForeignToplevelListV1::destructor(ext_foreign_toplevel_list_v1 *data) {
    ext_foreign_toplevel_list_v1_destroy(data);
}
#if defined(EXT_FOREIGN_TOPLEVEL_LIST_V1_STOP_SINCE_VERSION)
void ExtForeignToplevelListV1::stop() {
    ext_foreign_toplevel_list_v1_stop(*this);
}
#endif

} // namespace fcitx::wayland
