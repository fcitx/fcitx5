#ifndef WL_SHM
#define WL_SHM
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
namespace fcitx::wayland {
class WlShmPool;
class WlShm final {
public:
    static constexpr const char *interface = "wl_shm";
    static constexpr const wl_interface *const wlInterface = &wl_shm_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_shm wlType;
    operator wl_shm *() { return data_.get(); }
    WlShm(wlType *data);
    WlShm(WlShm &&other) noexcept = delete;
    WlShm &operator=(WlShm &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    WlShmPool *createPool(int32_t fd, int32_t size);
    auto &format() { return formatSignal_; }

private:
    static void destructor(wl_shm *);
    static const struct wl_shm_listener listener;
    fcitx::Signal<void(uint32_t)> formatSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_shm, &destructor> data_;
};
static inline wl_shm *rawPointer(WlShm *p) {
    return p ? static_cast<wl_shm *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
