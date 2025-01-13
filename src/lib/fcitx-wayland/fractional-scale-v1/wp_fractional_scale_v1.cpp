#include "wp_fractional_scale_v1.h"
#include <cassert>
namespace fcitx::wayland {
const struct wp_fractional_scale_v1_listener WpFractionalScaleV1::listener = {
    [](void *data, wp_fractional_scale_v1 *wldata, uint32_t scale) {
        auto *obj = static_cast<WpFractionalScaleV1 *>(data);
        assert(*obj == wldata);
        {
            return obj->preferredScale()(scale);
        }
    },
};
WpFractionalScaleV1::WpFractionalScaleV1(wp_fractional_scale_v1 *data)
    : version_(wp_fractional_scale_v1_get_version(data)), data_(data) {
    wp_fractional_scale_v1_set_user_data(*this, this);
    wp_fractional_scale_v1_add_listener(*this, &WpFractionalScaleV1::listener,
                                        this);
}
void WpFractionalScaleV1::destructor(wp_fractional_scale_v1 *data) {
    auto version = wp_fractional_scale_v1_get_version(data);
    if (version >= 1) {
        return wp_fractional_scale_v1_destroy(data);
    }
}
} // namespace fcitx::wayland
