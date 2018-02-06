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
#ifndef _FCITX_MODULES_DBUS_DBUS_PUBLIC_H_
#define _FCITX_MODULES_DBUS_DBUS_PUBLIC_H_

#include <fcitx-utils/dbus/bus.h>
#include <fcitx/addoninstance.h>

FCITX_ADDON_DECLARE_FUNCTION(DBusModule, bus, fcitx::dbus::Bus *());
FCITX_ADDON_DECLARE_FUNCTION(DBusModule, lockGroup, bool(int group));

#endif // _FCITX_MODULES_DBUS_DBUS_PUBLIC_H_
