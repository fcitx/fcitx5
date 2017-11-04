#ifndef WL_DATA_OFFER
#define WL_DATA_OFFER
#include "fcitx-utils/signals.h"
#include <memory>
#include <wayland-client.h>
namespace fcitx {
namespace wayland {
class WlDataOffer final {
public:
    static constexpr const char *interface = "wl_data_offer";
    static constexpr const wl_interface *const wlInterface =
        &wl_data_offer_interface;
    static constexpr const uint32_t version = 3;
    typedef wl_data_offer wlType;
    operator wl_data_offer *() { return data_.get(); }
    WlDataOffer(wlType *data);
    WlDataOffer(WlDataOffer &&other) noexcept = delete;
    WlDataOffer &operator=(WlDataOffer &&other) noexcept = delete;
    auto actualVersion() const { return version_; }
    void accept(uint32_t serial, const char *mimeType);
    void receive(const char *mimeType, int32_t fd);
    void finish();
    void setActions(uint32_t dndActions, uint32_t preferredAction);
    auto &offer() { return offerSignal_; }
    auto &sourceActions() { return sourceActionsSignal_; }
    auto &action() { return actionSignal_; }

private:
    static void destructor(wl_data_offer *);
    static const struct wl_data_offer_listener listener;
    fcitx::Signal<void(const char *)> offerSignal_;
    fcitx::Signal<void(uint32_t)> sourceActionsSignal_;
    fcitx::Signal<void(uint32_t)> actionSignal_;
    uint32_t version_;
    std::unique_ptr<wl_data_offer, decltype(&destructor)> data_;
};
static inline wl_data_offer *rawPointer(WlDataOffer *p) {
    return p ? static_cast<wl_data_offer *>(*p) : nullptr;
}
}
}
#endif
