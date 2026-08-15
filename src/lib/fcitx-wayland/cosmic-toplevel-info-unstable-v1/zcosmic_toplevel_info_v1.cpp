#include "zcosmic_toplevel_info_v1.h"
#include <cassert>
#include <wayland-client-core.h>
#include "ext_foreign_toplevel_handle_v1.h"
#include "wayland-cosmic-toplevel-info-unstable-v1-client-protocol.h"
#include "zcosmic_toplevel_handle_v1.h"

namespace fcitx::wayland {
const struct zcosmic_toplevel_info_v1_listener ZcosmicToplevelInfoV1::listener =
    {
#if defined(ZCOSMIC_TOPLEVEL_INFO_V1_TOPLEVEL_SINCE_VERSION)
        .toplevel =
            [](void *data, zcosmic_toplevel_info_v1 *wldata,
               zcosmic_toplevel_handle_v1 *toplevel) {
                auto *obj = static_cast<ZcosmicToplevelInfoV1 *>(data);
                assert(*obj == wldata);
                {
                    auto *toplevel_ = new ZcosmicToplevelHandleV1(toplevel);
                    obj->toplevel()(toplevel_);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_INFO_V1_FINISHED_SINCE_VERSION)
        .finished =
            [](void *data, zcosmic_toplevel_info_v1 *wldata) {
                auto *obj = static_cast<ZcosmicToplevelInfoV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->finished()();
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_INFO_V1_DONE_SINCE_VERSION)
        .done =
            [](void *data, zcosmic_toplevel_info_v1 *wldata) {
                auto *obj = static_cast<ZcosmicToplevelInfoV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->done()();
                }
            },
#endif
};

ZcosmicToplevelInfoV1::ZcosmicToplevelInfoV1(zcosmic_toplevel_info_v1 *data)
    : version_(zcosmic_toplevel_info_v1_get_version(data)), data_(data) {
    zcosmic_toplevel_info_v1_set_user_data(*this, this);
    zcosmic_toplevel_info_v1_add_listener(
        *this, &ZcosmicToplevelInfoV1::listener, this);
}

void ZcosmicToplevelInfoV1::destructor(zcosmic_toplevel_info_v1 *data) {
    zcosmic_toplevel_info_v1_destroy(data);
}
#if defined(ZCOSMIC_TOPLEVEL_INFO_V1_STOP_SINCE_VERSION)
void ZcosmicToplevelInfoV1::stop() { zcosmic_toplevel_info_v1_stop(*this); }
#endif
#if defined(ZCOSMIC_TOPLEVEL_INFO_V1_GET_COSMIC_TOPLEVEL_SINCE_VERSION)
ZcosmicToplevelHandleV1 *ZcosmicToplevelInfoV1::getCosmicToplevel(
    ExtForeignToplevelHandleV1 *foreignToplevel) {
    return new ZcosmicToplevelHandleV1(
        zcosmic_toplevel_info_v1_get_cosmic_toplevel(
            *this, rawPointer(foreignToplevel)));
}
#endif

} // namespace fcitx::wayland
