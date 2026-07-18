#include "wl_data_source.h"
#include <cassert>

namespace fcitx::wayland {
const struct wl_data_source_listener WlDataSource::listener = {
#if defined(WL_DATA_SOURCE_TARGET_SINCE_VERSION)
    .target =
        [](void *data, wl_data_source *wldata, const char *mimeType) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->target()(mimeType);
            }
        },
#endif
#if defined(WL_DATA_SOURCE_SEND_SINCE_VERSION)
    .send =
        [](void *data, wl_data_source *wldata, const char *mimeType,
           int32_t fd) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->send()(mimeType, fd);
            }
        },
#endif
#if defined(WL_DATA_SOURCE_CANCELLED_SINCE_VERSION)
    .cancelled =
        [](void *data, wl_data_source *wldata) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->cancelled()();
            }
        },
#endif
#if defined(WL_DATA_SOURCE_DND_DROP_PERFORMED_SINCE_VERSION)
    .dnd_drop_performed =
        [](void *data, wl_data_source *wldata) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->dndDropPerformed()();
            }
        },
#endif
#if defined(WL_DATA_SOURCE_DND_FINISHED_SINCE_VERSION)
    .dnd_finished =
        [](void *data, wl_data_source *wldata) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->dndFinished()();
            }
        },
#endif
#if defined(WL_DATA_SOURCE_ACTION_SINCE_VERSION)
    .action =
        [](void *data, wl_data_source *wldata, uint32_t dndAction) {
            auto *obj = static_cast<WlDataSource *>(data);
            assert(*obj == wldata);
            {
                obj->action()(dndAction);
            }
        },
#endif
};

WlDataSource::WlDataSource(wl_data_source *data)
    : version_(wl_data_source_get_version(data)), data_(data) {
    wl_data_source_set_user_data(*this, this);
    wl_data_source_add_listener(*this, &WlDataSource::listener, this);
}

void WlDataSource::destructor(wl_data_source *data) {
    wl_data_source_destroy(data);
}
#if defined(WL_DATA_SOURCE_OFFER_SINCE_VERSION)
void WlDataSource::offer(const char *mimeType) {
    wl_data_source_offer(*this, mimeType);
}
#endif
#if defined(WL_DATA_SOURCE_SET_ACTIONS_SINCE_VERSION)
void WlDataSource::setActions(uint32_t dndActions) {
    wl_data_source_set_actions(*this, dndActions);
}
#endif

} // namespace fcitx::wayland
