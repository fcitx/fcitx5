/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVERBASE_H_
#define _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVERBASE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <wayland-client-core.h>
#include <xkbcommon/xkbcommon.h>
#include "fcitx-utils/key.h"
#include "fcitx-utils/misc.h"
#include "fcitx/focusgroup.h"
#include "display.h"
#include "waylandim.h"
#include "wl_seat.h"

namespace fcitx {

typedef std::tuple<xkb_mod_mask_t,
                   xkb_mod_mask_t,
                   xkb_mod_mask_t,
                   xkb_layout_index_t>
WlModifiersParams;

class WaylandIMServerBase {
public:
    WaylandIMServerBase(wl_display *display, FocusGroup *group,
                        std::string name, WaylandIMModule *waylandim);
    virtual ~WaylandIMServerBase() = default;

    auto *parent() { return parent_; }
    auto *display() { return display_; }

    std::optional<std::string> mayCommitAsText(const Key &key,
                                               uint32_t state) const;

    std::optional<WlModifiersParams> mayChangeModifiers(
        const xkb_keycode_t key,
        const uint32_t state) const;

    int32_t repeatRate(
        const std::shared_ptr<wayland::WlSeat> &seat,
        const std::optional<std::tuple<int32_t, int32_t>> &defaultValue) const;
    int32_t repeatDelay(
        const std::shared_ptr<wayland::WlSeat> &seat,
        const std::optional<std::tuple<int32_t, int32_t>> &defaultValue) const;

protected:
    void updateModMasksMappings();

    FocusGroup *group_;
    std::string name_;
    WaylandIMModule *parent_;
    wayland::Display *display_;

    UniqueCPtr<struct xkb_context, xkb_context_unref> context_;
    UniqueCPtr<struct xkb_keymap, xkb_keymap_unref> keymap_;
    UniqueCPtr<struct xkb_state, xkb_state_unref> state_;

    KeyStates modifiers_;

    std::unordered_map<xkb_keycode_t, std::vector<std::tuple<
        xkb_mod_mask_t,
        WlModifiersParams>>>
    keycodeToNormalModMasks_;

    std::unordered_map<xkb_keycode_t, std::vector<std::tuple<
        xkb_mod_mask_t,
        WlModifiersParams,
        WlModifiersParams>>>
    keycodeToLockModMasks_;

private:
    std::optional<std::tuple<int32_t, int32_t>> repeatInfo(
        const std::shared_ptr<wayland::WlSeat> &seat,
        const std::optional<std::tuple<int32_t, int32_t>> &defaultValue) const;
};

} // namespace fcitx

#endif
