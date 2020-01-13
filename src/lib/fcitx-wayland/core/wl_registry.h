#ifndef WL_REGISTRY
#define WL_REGISTRY
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlRegistry final {
public:
    static constexpr const char *interface = "wl_registry";
    static constexpr const wl_interface *const wlInterface =
        &wl_registry_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_registry wlType;
    operator wl_registry *() { return data_.get(); }
    WlRegistry(wlType *data);
    WlRegistry(WlRegistry &&other) noexcept = delete;
    WlRegistry &operator=(WlRegistry &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    template <typename T>
    T *bind(uint32_t name, uint32_t requested_version) {
        return new T(static_cast<typename T::wlType *>(
            wl_registry_bind(*this, name, T::wlInterface, requested_version)));
    }
    auto &global() { return globalSignal_; }
    auto &globalRemove() { return globalRemoveSignal_; }

private:
    static void destructor(wl_registry *);
    static const struct wl_registry_listener listener;
    fcitx::Signal<void(uint32_t, const char *, uint32_t)> globalSignal_;
    fcitx::Signal<void(uint32_t)> globalRemoveSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    std::unique_ptr<wl_registry, decltype(&destructor)> data_;
};
static inline wl_registry *rawPointer(WlRegistry *p) {
    return p ? static_cast<wl_registry *>(*p) : nullptr;
}
} // namespace wayland
} // namespace fcitx
#endif
