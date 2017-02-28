#ifndef WL_REGISTRY
#define WL_REGISTRY
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlRegistry {
public:
    static constexpr const char *interface = "wl_registry";
    static constexpr const wl_interface *const wlInterface = &wl_registry_interface;
    static constexpr const uint32_t version = 1;
    typedef wl_registry wlType;
    operator wl_registry *() { return data_.get(); }
    WlRegistry(wlType *data);
    WlRegistry(WlRegistry &&other) : data_(std::move(other.data_)) {}
    WlRegistry &operator=(WlRegistry &&other) {
        data_ = std::move(other.data_);
        return *this;
    }
    auto actualVersion() const { return version_; }
    template <typename T>
    T *bind(uint32_t name) {
        return new T(static_cast<typename T::wlType *>(wl_registry_bind(*this, name, T::wlInterface, T::version)));
    }
    auto &global() { return globalSignal_; }
    auto &globalRemove() { return globalRemoveSignal_; }

private:
    static void destructor(wl_registry *);
    static const struct wl_registry_listener listener;
    fcitx::Signal<void(uint32_t, const char *, uint32_t)> globalSignal_;
    fcitx::Signal<void(uint32_t)> globalRemoveSignal_;
    uint32_t version_;
    std::unique_ptr<wl_registry, decltype(&destructor)> data_;
};
}
}
#endif
