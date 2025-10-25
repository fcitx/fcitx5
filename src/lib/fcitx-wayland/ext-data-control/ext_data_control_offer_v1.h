#ifndef EXT_DATA_CONTROL_OFFER_V1
#define EXT_DATA_CONTROL_OFFER_V1
#include <memory>
#include <wayland-client.h>
#include "fcitx-utils/signals.h"
#include "wayland-ext-data-control-unstable-v1-client-protocol.h"
namespace fcitx::wayland {
class ExtDataControlOfferV1 final {
public:
    static constexpr const char *interface = "ext_data_control_offer_v1";
    static constexpr const wl_interface *const wlInterface =
        &ext_data_control_offer_v1_interface;
    static constexpr const uint32_t version = 1;
    typedef ext_data_control_offer_v1 wlType;
    operator ext_data_control_offer_v1 *() { return data_.get(); }
    ExtDataControlOfferV1(wlType *data);
    ExtDataControlOfferV1(ExtDataControlOfferV1 &&other) noexcept = delete;
    ExtDataControlOfferV1 &
    operator=(ExtDataControlOfferV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void receive(const char *mimeType, int32_t fd);
    auto &offer() { return offerSignal_; }

private:
    static void destructor(ext_data_control_offer_v1 *);
    static const struct ext_data_control_offer_v1_listener listener;
    fcitx::Signal<void(const char *)> offerSignal_;
    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<ext_data_control_offer_v1, &destructor> data_;
};
static inline ext_data_control_offer_v1 *rawPointer(ExtDataControlOfferV1 *p) {
    return p ? static_cast<ext_data_control_offer_v1 *>(*p) : nullptr;
}
} // namespace fcitx::wayland
#endif
