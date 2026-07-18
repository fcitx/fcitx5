#include "wl_shm_pool.h"
#include "wl_buffer.h"

namespace fcitx::wayland {

WlShmPool::WlShmPool(wl_shm_pool *data)
    : version_(wl_shm_pool_get_version(data)), data_(data) {
    wl_shm_pool_set_user_data(*this, this);
}

void WlShmPool::destructor(wl_shm_pool *data) { wl_shm_pool_destroy(data); }
#if defined(WL_SHM_POOL_CREATE_BUFFER_SINCE_VERSION)
WlBuffer *WlShmPool::createBuffer(int32_t offset, int32_t width, int32_t height,
                                  int32_t stride, uint32_t format) {
    return new WlBuffer(wl_shm_pool_create_buffer(*this, offset, width, height,
                                                  stride, format));
}
#endif
#if defined(WL_SHM_POOL_RESIZE_SINCE_VERSION)
void WlShmPool::resize(int32_t size) { wl_shm_pool_resize(*this, size); }
#endif

} // namespace fcitx::wayland
