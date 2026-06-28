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

void updateModMasksMappingsForKey(
    struct xkb_keymap *keymap,
    struct xkb_state *state,
    xkb_keycode_t key,
    xkb_layout_index_t layout,
    xkb_level_index_t level,
    std::unordered_map<xkb_keycode_t, std::vector<std::tuple<
        xkb_mod_mask_t,
        fcitx::WlModifiersParams>>>
    &keycodeToNormalModMasks,
    std::unordered_map<xkb_keycode_t, std::vector<std::tuple<
        xkb_mod_mask_t,
        fcitx::WlModifiersParams,
        fcitx::WlModifiersParams>>>
    &keycodeToLockModMasks);

std::vector<xkb_mod_mask_t> getKeyModMasks(
    struct xkb_keymap *keymap,
    xkb_keycode_t key,
    xkb_layout_index_t layout,
    xkb_level_index_t level);

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

void WaylandIMServerBase::updateModMasksMappings() {
    if (!keymap_) {
        return;
    }

    std::unordered_map<xkb_keycode_t, std::vector<std::tuple<
        xkb_mod_mask_t,
        WlModifiersParams>>>
    keycodeToNormalModMasks;

    std::unordered_map<xkb_keycode_t, std::vector<std::tuple<
        xkb_mod_mask_t,
        WlModifiersParams,
        WlModifiersParams>>>
    keycodeToLockModMasks;

    // used for calculating modifier masks.
    struct xkb_state* state = xkb_state_new(keymap_.get());

    xkb_keycode_t min = xkb_keymap_min_keycode(keymap_.get());
    xkb_keycode_t max = xkb_keymap_max_keycode(keymap_.get());

    for (auto key = min; key <= max; ++key) {
        xkb_layout_index_t layouts = xkb_keymap_num_layouts_for_key(
            keymap_.get(), key);
        for (xkb_layout_index_t layout = 0; layout < layouts; ++layout) {
            xkb_level_index_t levels = xkb_keymap_num_levels_for_key(
                keymap_.get(), key, layout);
            for (xkb_level_index_t level = 0; level < levels; ++level) {
                updateModMasksMappingsForKey(
                    keymap_.get(),
                    state,
                    key,
                    layout,
                    level,
                    keycodeToNormalModMasks,
                    keycodeToLockModMasks);
            }
        }
    }

    xkb_state_unref(state);

    keycodeToNormalModMasks_ = keycodeToNormalModMasks;
    keycodeToLockModMasks_ = keycodeToLockModMasks;
}

std::optional<WlModifiersParams> WaylandIMServerBase::mayChangeModifiers(
    const xkb_keycode_t key,
    const uint32_t state) const {
    if (!keymap_ || !state_) {
        return std::nullopt;
    }

    xkb_mod_mask_t modsDepressed, modsLatched, modsLocked, modsEffective;
    xkb_layout_index_t layout;
    modsDepressed = xkb_state_serialize_mods(state_.get(),
                                             XKB_STATE_MODS_DEPRESSED);
    modsLatched = xkb_state_serialize_mods(state_.get(),
                                           XKB_STATE_MODS_LATCHED);
    modsLocked = xkb_state_serialize_mods(state_.get(), XKB_STATE_MODS_LOCKED);
    modsEffective = xkb_state_serialize_mods(state_.get(),
                                             XKB_STATE_MODS_EFFECTIVE);
    layout = xkb_state_serialize_layout(state_.get(), XKB_STATE_LAYOUT_LOCKED);

    if (auto it = keycodeToNormalModMasks_.find(key);
             it != keycodeToNormalModMasks_.end()) {
        for (auto const &modMasks : it->second) {
            if ((modsEffective & std::get<0>(modMasks)) != std::get<0>(modMasks)) {
                continue;
            }
            const WlModifiersParams &params = std::get<1>(modMasks);
            if (layout != std::get<3>(params)) {
                continue;
            }
            if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                return std::optional<WlModifiersParams>({
                    modsDepressed | (std::get<0>(params)),
                    modsLatched | (std::get<1>(params)),
                    modsLocked | (std::get<2>(params)),
                    layout});
            } else {
                return std::optional<WlModifiersParams>({
                    modsDepressed & (~std::get<0>(params)),
                    modsLatched & (~std::get<1>(params)),
                    modsLocked & (~std::get<2>(params)),
                    layout});
            }
        }
    }

    if (auto it = keycodeToLockModMasks_.find(key);
             it != keycodeToLockModMasks_.end()) {
        for (auto const &modMasks : it->second) {
            if ((modsEffective & std::get<0>(modMasks)) == 0) {
                continue;
            }

            const WlModifiersParams &pressedParams = std::get<1>(modMasks);
            if (layout != std::get<3>(pressedParams)) {
                continue;
            }
            const xkb_mod_mask_t downModsDepressed = std::get<0>(pressedParams);
            const xkb_mod_mask_t downModsLatched = std::get<1>(pressedParams);
            const xkb_mod_mask_t downModsLocked = std::get<2>(pressedParams);

            const WlModifiersParams &releasedParams = std::get<2>(modMasks);
            const xkb_mod_mask_t upModsDepressed = std::get<0>(releasedParams);
            const xkb_mod_mask_t upModsLatched = std::get<1>(releasedParams);
            const xkb_mod_mask_t upModsLocked = std::get<2>(releasedParams);

            if (((upModsDepressed & modsDepressed) == upModsDepressed) &&
                ((upModsLatched & modsLatched) == upModsLatched) &&
                ((upModsLocked & modsLocked) == upModsLocked)) {
                if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                    // the second time pressing the key.
                    // the modifiers is the same as the first pressing.
                    return std::optional<WlModifiersParams>({
                        modsDepressed | downModsDepressed,
                        modsLatched | downModsLatched,
                        modsLocked | downModsLocked,
                        layout});
                } else {
                    // releasing in the first time pressing.
                    return std::optional<WlModifiersParams>({
                        (modsDepressed & (~downModsDepressed)) |
                        upModsDepressed,
                        (modsLatched & (~downModsLatched)) | upModsLatched,
                        (modsLocked & (~downModsLocked)) | upModsLocked,
                        layout});
                }
            } else {
                if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                    // the first time pressing the key.
                    return std::optional<WlModifiersParams>({
                        modsDepressed | downModsDepressed,
                        modsLatched | downModsLatched,
                        modsLocked | downModsLocked,
                        layout});
                } else {
                    // releasing in the second time pressing.
                    // reset all related masks.
                    return std::optional<WlModifiersParams>({
                        modsDepressed & (~downModsDepressed),
                        modsLatched & (~downModsLatched),
                        modsLocked & (~downModsLocked),
                        layout});
                }
            }
        }
    }

    return std::nullopt;
}

} // namespace fcitx

void updateModMasksMappingsForKey(
    struct xkb_keymap *keymap,
    struct xkb_state *state,
    xkb_keycode_t key,
    xkb_layout_index_t layout,
    xkb_level_index_t level,
    std::unordered_map<xkb_keycode_t, std::vector<std::tuple<
        xkb_mod_mask_t,
        fcitx::WlModifiersParams>>>
    &keycodeToNormalModMasks,
    std::unordered_map<xkb_keycode_t, std::vector<std::tuple<
        xkb_mod_mask_t,
        fcitx::WlModifiersParams,
        fcitx::WlModifiersParams>>>
    &keycodeToLockModMasks) {

    auto modMasks = getKeyModMasks(keymap, key, layout, level);
    if (!keycodeToNormalModMasks.contains(key)) {
        keycodeToNormalModMasks[key] = std::vector<std::tuple<
            xkb_mod_mask_t,
            fcitx::WlModifiersParams>>();
    }
    if (!keycodeToLockModMasks.contains(key)) {
        keycodeToLockModMasks[key] = std::vector<std::tuple<
            xkb_mod_mask_t,
            fcitx::WlModifiersParams,
            fcitx::WlModifiersParams>>();
    }

    xkb_mod_mask_t modsDepressed, modsLatched, modsLocked;
    xkb_mod_mask_t downModsDepressed, downModsLatched, downModsLocked;
    xkb_mod_mask_t upModsDepressed, upModsLatched, upModsLocked;

    for (auto modMask : modMasks) {
        // check if pressing this key will modify the state of modifiers under
        // the certain state of modifiers.
        xkb_state_update_mask(state, modMask, 0, 0, 0, 0, layout);
        modsDepressed = xkb_state_serialize_mods(state,
                                                 XKB_STATE_MODS_DEPRESSED);
        modsLatched = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
        modsLocked = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);

        enum xkb_state_component downComponentMask = xkb_state_update_key(
            state, key, XKB_KEY_DOWN);
        downModsDepressed = xkb_state_serialize_mods(state,
                                                     XKB_STATE_MODS_DEPRESSED);
        downModsLatched = xkb_state_serialize_mods(state,
                                                   XKB_STATE_MODS_LATCHED);
        downModsLocked = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);

        enum xkb_state_component upComponentMask = xkb_state_update_key(
            state, key, XKB_KEY_UP);
        upModsDepressed = xkb_state_serialize_mods(state,
                                                   XKB_STATE_MODS_DEPRESSED);
        upModsLatched = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
        upModsLocked = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);

        if (downComponentMask != 0 || upComponentMask != 0) {
            if (upModsDepressed == modsDepressed &&
                upModsLatched == modsLatched &&
                upModsLocked == modsLocked) {
                keycodeToNormalModMasks[key].push_back({
                    modMask,
                    {
                        downModsDepressed ^ modsDepressed,
                        downModsLatched ^ modsLatched,
                        downModsLocked ^ modsLocked,
                        layout}});
            } else {
                keycodeToLockModMasks[key].push_back({
                    modMask,
                    {
                        downModsDepressed ^ modsDepressed,
                        downModsLatched ^ modsLatched,
                        downModsLocked ^ modsLocked,
                        layout},
                    {
                        upModsDepressed ^ modsDepressed,
                        upModsLatched ^ modsLatched,
                        upModsLocked ^ modsLocked,
                        layout}});
            }
        }
    }
}

std::vector<xkb_mod_mask_t> getKeyModMasks(
    struct xkb_keymap *keymap,
    xkb_keycode_t key,
    xkb_layout_index_t layout,
    xkb_level_index_t level) {

    std::vector<xkb_mod_mask_t> out;
    size_t masks_size = 4;
    xkb_mod_index_t* masks = NULL;
    while (true) {
        if (masks) {
            delete[] masks;
        }
        masks = new xkb_mod_index_t[masks_size];
        size_t num = xkb_keymap_key_get_mods_for_level(
            keymap, key, layout, level, masks, masks_size);
        if (num == masks_size) {
            // if num equals to masks_size, it may contain more items.
            masks_size <<= 1;
            continue;
        }
        masks_size = num;
        break;
    }

    if (masks) {
        for (size_t i = 0; i < masks_size; ++i) {
            out.push_back(masks[i]);
        }
        delete[] masks;
    }

    return out;
}
