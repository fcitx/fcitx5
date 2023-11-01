/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVERBASE_H_
#define _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIMSERVERBASE_H_

#include <string>
#include <wayland-client-core.h>
#include <xkbcommon/xkbcommon.h>
#include "fcitx-utils/event.h"
#include "fcitx-utils/misc.h"
#include "fcitx/focusgroup.h"
#include "display.h"
#include "waylandim.h"

namespace fcitx {

class WaylandIMServerBase {
public:
    WaylandIMServerBase(wl_display *display, FocusGroup *group,
                        const std::string &name, WaylandIMModule *waylandim);
    virtual ~WaylandIMServerBase() = default;

    void deferredFlush();

protected:
    FocusGroup *group_;
    std::string name_;
    WaylandIMModule *parent_;
    wayland::Display *display_;

    std::unique_ptr<EventSource> deferEvent_;

    UniqueCPtr<struct xkb_context, xkb_context_unref> context_;
    UniqueCPtr<struct xkb_keymap, xkb_keymap_unref> keymap_;
    UniqueCPtr<struct xkb_state, xkb_state_unref> state_;

    KeyStates modifiers_;
};

} // namespace fcitx

#endif
