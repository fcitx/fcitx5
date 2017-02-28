#ifndef WL_SHM_POOL
#define WL_SHM_POOL
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlBuffer;
class WlShmPool {
public:
    static constexpr const char *interface = "wl_shm_pool";
    static constexpr const wl_interface *const wlInterface = &wl_shm_pool_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_shm_pool wlType;
    operator wl_shm_pool *() { return data_.get(); }
    WlShmPool(wlType *data);
    WlShmPool(WlShmPool &&other) : data_(std::move(other.data_)) {}
    WlShmPool &operator=(WlShmPool &&other) {
        data_ = std::move(other.data_);
        return *this;
    }
    auto actualVersion() const { return version_; }
    WlBuffer *createBuffer(int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format);
    void resize(int32_t size);

private:
    static void destructor(wl_shm_pool *);
    uint32_t version_;
    std::unique_ptr<wl_shm_pool, decltype(&destructor)> data_;
};
}
}
#endif
