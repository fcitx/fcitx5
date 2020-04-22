//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
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
