#ifndef EXT_BACKGROUND_EFFECT_SURFACE_V1_H_
#define EXT_BACKGROUND_EFFECT_SURFACE_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "wayland-ext-background-effect-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class WlRegion;

class ExtBackgroundEffectSurfaceV1 final {
public:
    static constexpr const char *interface = "ext_background_effect_surface_v1";
    static constexpr const wl_interface *const wlInterface =
        &ext_background_effect_surface_v1_interface;
    static constexpr const uint32_t version = 1;
    using wlType = ext_background_effect_surface_v1;
    operator ext_background_effect_surface_v1 *() { return data_.get(); }
    ExtBackgroundEffectSurfaceV1(wlType *data);
    ExtBackgroundEffectSurfaceV1(
        ExtBackgroundEffectSurfaceV1 &&other) noexcept = delete;
    ExtBackgroundEffectSurfaceV1 &
    operator=(ExtBackgroundEffectSurfaceV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void setBlurRegion(WlRegion *region);

private:
    static void destructor(ext_background_effect_surface_v1 *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<ext_background_effect_surface_v1, &destructor> data_;
};
static inline ext_background_effect_surface_v1 *
rawPointer(ExtBackgroundEffectSurfaceV1 *p) {
    return p ? static_cast<ext_background_effect_surface_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // EXT_BACKGROUND_EFFECT_SURFACE_V1_H_
