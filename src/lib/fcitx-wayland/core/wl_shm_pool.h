#ifndef WL_SHM_POOL
#define WL_SHM_POOL
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx {
namespace wayland {
class WlBuffer;
class WlShmPool final {
public:
    static constexpr const char *interface = "wl_shm_pool";
    static constexpr const wl_interface *const wlInterface =
        &wl_shm_pool_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_shm_pool wlType;
    operator wl_shm_pool *() { return data_.get(); }
    WlShmPool(wlType *data);
    WlShmPool(WlShmPool &&other) noexcept = delete;
    WlShmPool &operator=(WlShmPool &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WlBuffer *createBuffer(int32_t offset, int32_t width, int32_t height,
                           int32_t stride, uint32_t format);
    void resize(int32_t size);

private:
    static void destructor(wl_shm_pool *);
    uint32_t version_;
    void *userData_ = nullptr;
    std::unique_ptr<wl_shm_pool, decltype(&destructor)> data_;
};
static inline wl_shm_pool *rawPointer(WlShmPool *p) {
    return p ? static_cast<wl_shm_pool *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
