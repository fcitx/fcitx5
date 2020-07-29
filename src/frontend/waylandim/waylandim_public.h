/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIM_PUBLIC_H_
#define _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIM_PUBLIC_H_

#include <functional>
#include <memory>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance.h>
#include <fcitx/focusgroup.h>
#include <wayland-client.h>
#include "zwp_input_method_v2.h"

FCITX_ADDON_DECLARE_FUNCTION(
    WaylandIMModule, getInputMethodV2,
    fcitx::wayland::ZwpInputMethodV2 *(fcitx::InputContext *));

#endif // _FCITX5_FRONTEND_WAYLANDIM_WAYLANDIM_PUBLIC_H_
