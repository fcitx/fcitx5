#ifndef EXT_BACKGROUND_EFFECT_MANAGER_V1_H_
#define EXT_BACKGROUND_EFFECT_MANAGER_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-ext-background-effect-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class ExtBackgroundEffectSurfaceV1;
class WlSurface;

class ExtBackgroundEffectManagerV1 final {
public:
    static constexpr const char *interface = "ext_background_effect_manager_v1";
    static constexpr const wl_interface *const wlInterface =
        &ext_background_effect_manager_v1_interface;
    static constexpr const uint32_t version = 1;
    using wlType = ext_background_effect_manager_v1;
    operator ext_background_effect_manager_v1 *() { return data_.get(); }
    ExtBackgroundEffectManagerV1(wlType *data);
    ExtBackgroundEffectManagerV1(
        ExtBackgroundEffectManagerV1 &&other) noexcept = delete;
    ExtBackgroundEffectManagerV1 &
    operator=(ExtBackgroundEffectManagerV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    ExtBackgroundEffectSurfaceV1 *getBackgroundEffect(WlSurface *surface);

    auto &capabilities() { return capabilitiesSignal_; }

private:
    static void destructor(ext_background_effect_manager_v1 *);
    static const struct ext_background_effect_manager_v1_listener listener;
    fcitx::Signal<void(uint32_t)> capabilitiesSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<ext_background_effect_manager_v1, &destructor> data_;
};
static inline ext_background_effect_manager_v1 *
rawPointer(ExtBackgroundEffectManagerV1 *p) {
    return p ? static_cast<ext_background_effect_manager_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // EXT_BACKGROUND_EFFECT_MANAGER_V1_H_
