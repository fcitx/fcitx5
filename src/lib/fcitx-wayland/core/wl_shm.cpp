#include "wl_shm.h"
#include "wl_shm_pool.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlShm::interface;
constexpr const wl_interface *const WlShm::wlInterface;
const uint32_t WlShm::version;
const struct wl_shm_listener WlShm::listener = {
    [](void *data, wl_shm *wldata, uint32_t format) {
        auto obj = static_cast<WlShm *>(data);
        assert(*obj == wldata);
        { return obj->format()(format); }
    },
};
WlShm::WlShm(wl_shm *data) : version_(wl_shm_get_version(data)), data_(data, &WlShm::destructor) {
    wl_shm_set_user_data(*this, this);
    wl_shm_add_listener(*this, &WlShm::listener, this);
}
void WlShm::destructor(wl_shm *data) {
    { return wl_shm_destroy(data); }
}
WlShmPool *WlShm::createPool(int32_t fd, int32_t size) { return new WlShmPool(wl_shm_create_pool(*this, fd, size)); }
}
}
