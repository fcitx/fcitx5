/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVERBASE_H_
#define _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVERBASE_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <wayland-client-core.h>
#include <xkbcommon/xkbcommon.h>
#include "fcitx-utils/key.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/focusgroup.h"
#include "display.h"
#include "waylandim.h"
#include "wl_seat.h"

namespace fcitx {

class WaylandIMServerBase {
public:
    WaylandIMServerBase(wl_display *display, FocusGroup *group,
                        std::string name, WaylandIMModule *waylandim);
    virtual ~WaylandIMServerBase() = default;

    auto *parent() { return parent_; }
    auto *display() { return display_; }

    std::optional<std::string> mayCommitAsText(const Key &key,
                                               uint32_t state) const;

    int32_t repeatRate(
        const std::shared_ptr<wayland::WlSeat> &seat,
        const std::optional<std::tuple<int32_t, int32_t>> &defaultValue) const;
    int32_t repeatDelay(
        const std::shared_ptr<wayland::WlSeat> &seat,
        const std::optional<std::tuple<int32_t, int32_t>> &defaultValue) const;

    template <typename Callback>
    static void commitStringWrapper(const std::string &text,
                                    const Callback &callback) {
        if (text.size() < safeStringLimit) {
            callback(text.data());
            return;
        }
        commitStringWrapperImpl(text, callback);
    }

    // zwp_input_method_v2 mentioned that commit string should be less than 4000
    // bytes, use this number of avoid hit 4096 wayland message size limit.
    static constexpr size_t safeStringLimit = 4000;

protected:
    FocusGroup *group_;
    std::string name_;
    WaylandIMModule *parent_;
    wayland::Display *display_;

    UniqueCPtr<struct xkb_context, xkb_context_unref> context_;
    UniqueCPtr<struct xkb_keymap, xkb_keymap_unref> keymap_;
    UniqueCPtr<struct xkb_state, xkb_state_unref> state_;

    KeyStates modifiers_;

private:
    template <typename Callback>
    static void commitStringWrapperImpl(std::string text,
                                        const Callback &callback) {
        char *start = text.data();
        char *end = text.data() + text.length();

        while (start < end) {
            char *current = start;

            while (current < end) {
                uint32_t c;
                char *newCurrent = utf8::getNextChar(current, end, &c);
                if (!utf8::isValidChar(c)) {
                    return;
                }
                if (newCurrent > start + safeStringLimit) {
                    break;
                }
                assert(current < newCurrent);
                current = newCurrent;
            }
            assert(current <= end);
            assert(start < current);
            // Set *current to \0, so [start, current) will become a nul
            // terminate string.
            const char pivot = *current;
            *current = '\0';
            callback(start);
            // Restore after we use it.
            *current = pivot;
            start = current;
        }
    }

    std::optional<std::tuple<int32_t, int32_t>> repeatInfo(
        const std::shared_ptr<wayland::WlSeat> &seat,
        const std::optional<std::tuple<int32_t, int32_t>> &defaultValue) const;
};

} // namespace fcitx

#endif
