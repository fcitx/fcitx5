/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_DBUS_DBUS_PUBLIC_H_
#define _FCITX_MODULES_DBUS_DBUS_PUBLIC_H_

#include <fcitx-utils/dbus/bus.h>
#include <fcitx/addoninstance.h>

FCITX_ADDON_DECLARE_FUNCTION(DBusModule, bus, fcitx::dbus::Bus *());
FCITX_ADDON_DECLARE_FUNCTION(DBusModule, lockGroup, bool(int group));

#endif // _FCITX_MODULES_DBUS_DBUS_PUBLIC_H_
