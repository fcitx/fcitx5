#include "zwp_tablet_v2.h"
namespace fcitx::wayland {
const struct zwp_tablet_v2_listener ZwpTabletV2::listener = {
    [](void *data, zwp_tablet_v2 *wldata, const char *name) {
        auto *obj = static_cast<ZwpTabletV2 *>(data);
        assert(*obj == wldata);
        { return obj->name()(name); }
    },
    [](void *data, zwp_tablet_v2 *wldata, uint32_t vid, uint32_t pid) {
        auto *obj = static_cast<ZwpTabletV2 *>(data);
        assert(*obj == wldata);
        { return obj->id()(vid, pid); }
    },
    [](void *data, zwp_tablet_v2 *wldata, const char *path) {
        auto *obj = static_cast<ZwpTabletV2 *>(data);
        assert(*obj == wldata);
        { return obj->path()(path); }
    },
    [](void *data, zwp_tablet_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletV2 *>(data);
        assert(*obj == wldata);
        { return obj->done()(); }
    },
    [](void *data, zwp_tablet_v2 *wldata) {
        auto *obj = static_cast<ZwpTabletV2 *>(data);
        assert(*obj == wldata);
        { return obj->removed()(); }
    },
};
ZwpTabletV2::ZwpTabletV2(zwp_tablet_v2 *data)
    : version_(zwp_tablet_v2_get_version(data)), data_(data) {
    zwp_tablet_v2_set_user_data(*this, this);
    zwp_tablet_v2_add_listener(*this, &ZwpTabletV2::listener, this);
}
void ZwpTabletV2::destructor(zwp_tablet_v2 *data) {
    auto version = zwp_tablet_v2_get_version(data);
    if (version >= 1) {
        return zwp_tablet_v2_destroy(data);
    }
}
} // namespace fcitx::wayland
