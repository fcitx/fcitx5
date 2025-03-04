#include "zwp_tablet_manager_v2.h"
#include "wayland-tablet-unstable-v2-client-protocol.h"
#include "wl_seat.h"
#include "zwp_tablet_seat_v2.h"

namespace fcitx::wayland {

ZwpTabletManagerV2::ZwpTabletManagerV2(zwp_tablet_manager_v2 *data)
    : version_(zwp_tablet_manager_v2_get_version(data)), data_(data) {
    zwp_tablet_manager_v2_set_user_data(*this, this);
}

void ZwpTabletManagerV2::destructor(zwp_tablet_manager_v2 *data) {
    const auto version = zwp_tablet_manager_v2_get_version(data);
    if (version >= 1) {
        zwp_tablet_manager_v2_destroy(data);
        return;
    }
}
ZwpTabletSeatV2 *ZwpTabletManagerV2::getTabletSeat(WlSeat *seat) {
    return new ZwpTabletSeatV2(
        zwp_tablet_manager_v2_get_tablet_seat(*this, rawPointer(seat)));
}

} // namespace fcitx::wayland
