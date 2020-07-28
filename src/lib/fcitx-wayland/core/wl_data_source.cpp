#include "wl_data_source.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlDataSource::interface;
constexpr const wl_interface *const WlDataSource::wlInterface;
const uint32_t WlDataSource::version;
const struct wl_data_source_listener WlDataSource::listener = {
    [](void *data, wl_data_source *wldata, const char *mimeType) {
        auto obj = static_cast<WlDataSource *>(data);
        assert(*obj == wldata);
        { return obj->target()(mimeType); }
    },
    [](void *data, wl_data_source *wldata, const char *mimeType, int32_t fd) {
        auto obj = static_cast<WlDataSource *>(data);
        assert(*obj == wldata);
        { return obj->send()(mimeType, fd); }
    },
    [](void *data, wl_data_source *wldata) {
        auto obj = static_cast<WlDataSource *>(data);
        assert(*obj == wldata);
        { return obj->cancelled()(); }
    },
    [](void *data, wl_data_source *wldata) {
        auto obj = static_cast<WlDataSource *>(data);
        assert(*obj == wldata);
        { return obj->dndDropPerformed()(); }
    },
    [](void *data, wl_data_source *wldata) {
        auto obj = static_cast<WlDataSource *>(data);
        assert(*obj == wldata);
        { return obj->dndFinished()(); }
    },
    [](void *data, wl_data_source *wldata, uint32_t dndAction) {
        auto obj = static_cast<WlDataSource *>(data);
        assert(*obj == wldata);
        { return obj->action()(dndAction); }
    },
};
WlDataSource::WlDataSource(wl_data_source *data)
    : version_(wl_data_source_get_version(data)), data_(data) {
    wl_data_source_set_user_data(*this, this);
    wl_data_source_add_listener(*this, &WlDataSource::listener, this);
}
void WlDataSource::destructor(wl_data_source *data) {
    auto version = wl_data_source_get_version(data);
    if (version >= 1) {
        return wl_data_source_destroy(data);
    }
}
void WlDataSource::offer(const char *mimeType) {
    return wl_data_source_offer(*this, mimeType);
}
void WlDataSource::setActions(uint32_t dndActions) {
    return wl_data_source_set_actions(*this, dndActions);
}
} // namespace wayland
} // namespace fcitx
