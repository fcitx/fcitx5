#include "wl_shm_pool.h"
#include "wl_buffer.h"
#include <cassert>
namespace fcitx {
namespace wayland {
constexpr const char *WlShmPool::interface;
constexpr const wl_interface *const WlShmPool::wlInterface;
const uint32_t WlShmPool::version;
WlShmPool::WlShmPool(wl_shm_pool *data)
    : version_(wl_shm_pool_get_version(data)),
      data_(data, &WlShmPool::destructor) {
    wl_shm_pool_set_user_data(*this, this);
}
void WlShmPool::destructor(wl_shm_pool *data) {
    auto version = wl_shm_pool_get_version(data);
    if (version >= 1) {
        return wl_shm_pool_destroy(data);
    }
}
WlBuffer *WlShmPool::createBuffer(int32_t offset, int32_t width, int32_t height,
                                  int32_t stride, uint32_t format) {
    return new WlBuffer(wl_shm_pool_create_buffer(*this, offset, width, height,
                                                  stride, format));
}
void WlShmPool::resize(int32_t size) { return wl_shm_pool_resize(*this, size); }
}
}
