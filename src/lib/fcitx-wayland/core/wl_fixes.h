#ifndef WL_FIXES_H_
#define WL_FIXES_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
namespace fcitx::wayland {

class WlRegistry;

class WlFixes final {
public:
    static constexpr const char *interface = "wl_fixes";
    static constexpr const wl_interface *const wlInterface =
        &wl_fixes_interface;
    static constexpr const uint32_t version = 2;
    using wlType = wl_fixes;
    operator wl_fixes *() { return data_.get(); }
    WlFixes(wlType *data);
    WlFixes(WlFixes &&other) noexcept = delete;
    WlFixes &operator=(WlFixes &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void destroyRegistry(WlRegistry *registry);
    void ackGlobalRemove(WlRegistry *registry, uint32_t name);

private:
    static void destructor(wl_fixes *);

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<wl_fixes, &destructor> data_;
};
static inline wl_fixes *rawPointer(WlFixes *p) {
    return p ? static_cast<wl_fixes *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // WL_FIXES_H_
