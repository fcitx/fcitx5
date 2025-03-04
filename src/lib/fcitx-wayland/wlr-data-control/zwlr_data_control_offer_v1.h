#ifndef ZWLR_DATA_CONTROL_OFFER_V1_H_
#define ZWLR_DATA_CONTROL_OFFER_V1_H_
#include <cstdint>
#include <wayland-client.h>
#include <wayland-util.h>
#include "fcitx-utils/misc.h"
#include "fcitx-utils/signals.h"
#include "wayland-wlr-data-control-unstable-v1-client-protocol.h" // IWYU pragma: export
namespace fcitx::wayland {

class ZwlrDataControlOfferV1 final {
public:
    static constexpr const char *interface = "zwlr_data_control_offer_v1";
    static constexpr const wl_interface *const wlInterface =
        &zwlr_data_control_offer_v1_interface;
    static constexpr const uint32_t version = 1;
    using wlType = zwlr_data_control_offer_v1;
    operator zwlr_data_control_offer_v1 *() { return data_.get(); }
    ZwlrDataControlOfferV1(wlType *data);
    ZwlrDataControlOfferV1(ZwlrDataControlOfferV1 &&other) noexcept = delete;
    ZwlrDataControlOfferV1 &
    operator=(ZwlrDataControlOfferV1 &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void *userData() const { return userData_; }
    void setUserData(void *userData) { userData_ = userData; }
    void receive(const char *mimeType, int32_t fd);

    auto &offer() { return offerSignal_; }

private:
    static void destructor(zwlr_data_control_offer_v1 *);
    static const struct zwlr_data_control_offer_v1_listener listener;
    fcitx::Signal<void(const char *)> offerSignal_;

    uint32_t version_;
    void *userData_ = nullptr;
    UniqueCPtr<zwlr_data_control_offer_v1, &destructor> data_;
};
static inline zwlr_data_control_offer_v1 *
rawPointer(ZwlrDataControlOfferV1 *p) {
    return p ? static_cast<zwlr_data_control_offer_v1 *>(*p) : nullptr;
}

} // namespace fcitx::wayland

#endif // ZWLR_DATA_CONTROL_OFFER_V1_H_
