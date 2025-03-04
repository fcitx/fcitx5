#include "wl_data_source.h"
#include <cassert>

namespace fcitx::wayland {
const struct wl_data_source_listener WlDataSource::listener = {
    .target =
        [](void *data, wl_data_source *wldata, const char *mimeType) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->target()(mimeType);
            }
        },
    .send =
        [](void *data, wl_data_source *wldata, const char *mimeType,
           int32_t fd) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->send()(mimeType, fd);
            }
        },
    .cancelled =
        [](void *data, wl_data_source *wldata) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->cancelled()();
            }
        },
    .dnd_drop_performed =
        [](void *data, wl_data_source *wldata) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->dndDropPerformed()();
            }
        },
    .dnd_finished =
        [](void *data, wl_data_source *wldata) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->dndFinished()();
            }
        },
    .action =
        [](void *data, wl_data_source *wldata, uint32_t dndAction) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->action()(dndAction);
            }
        },
};

WlDataSource::WlDataSource(wl_data_source *data)
    : version_(wl_data_source_get_version(data)), data_(data) {
    wl_data_source_set_user_data(*this, this);
    wl_data_source_add_listener(*this, &WlDataSource::listener, this);
}

void WlDataSource::destructor(wl_data_source *data) {
    const auto version = wl_data_source_get_version(data);
    if (version >= 1) {
        wl_data_source_destroy(data);
        return;
    }
}
void WlDataSource::offer(const char *mimeType) {
    wl_data_source_offer(*this, mimeType);
}
void WlDataSource::setActions(uint32_t dndActions) {
    wl_data_source_set_actions(*this, dndActions);
}

} // namespace fcitx::wayland
