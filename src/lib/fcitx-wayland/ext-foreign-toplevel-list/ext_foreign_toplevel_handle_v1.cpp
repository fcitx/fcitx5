#include "ext_foreign_toplevel_handle_v1.h"
#include <cassert>
#include "wayland-ext-foreign-toplevel-list-v1-client-protocol.h"

namespace fcitx::wayland {
const struct ext_foreign_toplevel_handle_v1_listener
    ExtForeignToplevelHandleV1::listener = {
#if defined(EXT_FOREIGN_TOPLEVEL_HANDLE_V1_CLOSED_SINCE_VERSION)
        .closed =
            [](void *data, ext_foreign_toplevel_handle_v1 *wldata) {
                auto *obj = static_cast<ExtForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->closed()();
                }
            },
#endif
#if defined(EXT_FOREIGN_TOPLEVEL_HANDLE_V1_DONE_SINCE_VERSION)
        .done =
            [](void *data, ext_foreign_toplevel_handle_v1 *wldata) {
                auto *obj = static_cast<ExtForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->done()();
                }
            },
#endif
#if defined(EXT_FOREIGN_TOPLEVEL_HANDLE_V1_TITLE_SINCE_VERSION)
        .title =
            [](void *data, ext_foreign_toplevel_handle_v1 *wldata,
               const char *title) {
                auto *obj = static_cast<ExtForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->title()(title);
                }
            },
#endif
#if defined(EXT_FOREIGN_TOPLEVEL_HANDLE_V1_APP_ID_SINCE_VERSION)
        .app_id =
            [](void *data, ext_foreign_toplevel_handle_v1 *wldata,
               const char *appId) {
                auto *obj = static_cast<ExtForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->appId()(appId);
                }
            },
#endif
#if defined(EXT_FOREIGN_TOPLEVEL_HANDLE_V1_IDENTIFIER_SINCE_VERSION)
        .identifier =
            [](void *data, ext_foreign_toplevel_handle_v1 *wldata,
               const char *identifier) {
                auto *obj = static_cast<ExtForeignToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->identifier()(identifier);
                }
            },
#endif
};

ExtForeignToplevelHandleV1::ExtForeignToplevelHandleV1(
    ext_foreign_toplevel_handle_v1 *data)
    : version_(ext_foreign_toplevel_handle_v1_get_version(data)), data_(data) {
    ext_foreign_toplevel_handle_v1_set_user_data(*this, this);
    ext_foreign_toplevel_handle_v1_add_listener(
        *this, &ExtForeignToplevelHandleV1::listener, this);
}

void ExtForeignToplevelHandleV1::destructor(
    ext_foreign_toplevel_handle_v1 *data) {
    ext_foreign_toplevel_handle_v1_destroy(data);
}

} // namespace fcitx::wayland
