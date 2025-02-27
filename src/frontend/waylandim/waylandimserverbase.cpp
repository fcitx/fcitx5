/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "waylandimserverbase.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/focusgroup.h"
#include "display.h"
#include "wayland_public.h"
#include "waylandim.h"
#include "wl_seat.h"

namespace fcitx {

WaylandIMServerBase::WaylandIMServerBase(wl_display *display, FocusGroup *group,
                                         std::string name,
                                         WaylandIMModule *waylandim)
    : group_(group), name_(std::move(name)), parent_(waylandim),
      display_(
          static_cast<wayland::Display *>(wl_display_get_user_data(display))) {}

std::optional<std::string>
WaylandIMServerBase::mayCommitAsText(const Key &key, uint32_t state) const {
    KeyStates nonShiftMask =
        KeyStates(KeyState::SimpleMask) & (~KeyStates(KeyState::Shift));
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED &&
        !*parent_->config().preferKeyEvent) {
        auto utf32 = Key::keySymToUnicode(key.sym());
        bool chToIgnore = (utf32 == '\n' || utf32 == '\b' || utf32 == '\r' ||
                           utf32 == '\t' || utf32 == '\033' || utf32 == '\x7f');
        if (!key.states().testAny(nonShiftMask) && utf32 && !chToIgnore) {
            return utf8::UCS4ToUTF8(utf32);
        }
    }
    return std::nullopt;
}

std::optional<std::tuple<int32_t, int32_t>> WaylandIMServerBase::repeatInfo(
    const std::shared_ptr<wayland::WlSeat> &seat,
    const std::optional<std::tuple<int32_t, int32_t>> &defaultValue) const {
    if (defaultValue) {
        return defaultValue;
    }
    auto seatPtr = seat;
    if (!seatPtr) {
        seatPtr = display_->getGlobal<wayland::WlSeat>();
    }
    if (seatPtr) {
        auto repeatInfo = parent_->wayland()->call<IWaylandModule::repeatInfo>(
            name_, *seatPtr);
        if (repeatInfo) {
            return repeatInfo;
        }
    }
    return std::nullopt;
}

int32_t WaylandIMServerBase::repeatRate(
    const std::shared_ptr<wayland::WlSeat> &seat,
    const std::optional<std::tuple<int32_t, int32_t>> &defaultValue) const {
    if (auto info = repeatInfo(seat, defaultValue)) {
        return std::get<0>(info.value());
    }
    return 25;
}

int32_t WaylandIMServerBase::repeatDelay(
    const std::shared_ptr<wayland::WlSeat> &seat,
    const std::optional<std::tuple<int32_t, int32_t>> &defaultValue) const {
    if (auto info = repeatInfo(seat, defaultValue)) {
        return std::get<1>(info.value());
    }
    return 600;
}

} // namespace fcitx
