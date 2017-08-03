#ifndef WL_SHM
#define WL_SHM
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlShmPool;
class WlShm final {
public:
    static constexpr const char *interface = "wl_shm";
    static constexpr const wl_interface *const wlInterface = &wl_shm_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_shm wlType;
    operator wl_shm *() { return data_.get(); }
    WlShm(wlType *data);
    WlShm(WlShm &&other) noexcept = default;
    WlShm &operator=(WlShm &&other) noexcept = default;
    auto actualVersion() const { return version_; }
    WlShmPool *createPool(int32_t fd, int32_t size);
    auto &format() { return formatSignal_; }

private:
    static void destructor(wl_shm *);
    static const struct wl_shm_listener listener;
    fcitx::Signal<void(uint32_t)> formatSignal_;
    uint32_t version_;
    std::unique_ptr<wl_shm, decltype(&destructor)> data_;
};
}
}
#endif
