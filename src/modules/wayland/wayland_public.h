/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_WAYLAND_WAYLAND_PUBLIC_H_
#define _FCITX_MODULES_WAYLAND_WAYLAND_PUBLIC_H_

#include <functional>
#include <memory>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance.h>
#include <fcitx/focusgroup.h>
#include <wayland-client.h>

namespace fcitx {

typedef std::function<void(const std::string &name, wl_display *display,
                           FocusGroup *group)>
    WaylandConnectionCreated;
typedef std::function<void(const std::string &name, wl_display *display)>
    WaylandConnectionClosed;
} // namespace fcitx

FCITX_ADDON_DECLARE_FUNCTION(
    WaylandModule, addConnectionCreatedCallback,
    std::unique_ptr<HandlerTableEntry<fcitx::WaylandConnectionCreated>>(
        WaylandConnectionCreated));
FCITX_ADDON_DECLARE_FUNCTION(
    WaylandModule, addConnectionClosedCallback,
    std::unique_ptr<HandlerTableEntry<fcitx::WaylandConnectionClosed>>(
        WaylandConnectionClosed));

#endif // _FCITX_MODULES_WAYLAND_WAYLAND_PUBLIC_H_
