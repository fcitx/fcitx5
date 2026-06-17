/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIM_PUBLIC_H_
#define _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIM_PUBLIC_H_

#include <string>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance.h>
#include <fcitx/focusgroup.h>

namespace fcitx::wayland {
class ZwpInputMethodV2;
}

FCITX_ADDON_DECLARE_FUNCTION(
    WaylandIMModule, getInputMethodV2,
    fcitx::wayland::ZwpInputMethodV2 *(fcitx::InputContext *));

// Raw zwp_input_method_v2 accessor for external UI addons (no internal
// C++ wrapper needed): create the input popup surface yourself.
struct zwp_input_method_v2;
FCITX_ADDON_DECLARE_FUNCTION(
    WaylandIMModule, getInputMethodV2Raw,
    struct zwp_input_method_v2 *(fcitx::InputContext *));

FCITX_ADDON_DECLARE_FUNCTION(WaylandIMModule, hasKeyboardGrab,
                             bool(const std::string &display));

#endif // _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIM_PUBLIC_H_
