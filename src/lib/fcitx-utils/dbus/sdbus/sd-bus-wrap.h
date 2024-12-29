/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_SD_BUS_WRAP_H_
#define _FCITX_UTILS_DBUS_SD_BUS_WRAP_H_

#if defined(__COVERITY__) && !defined(__INCLUDE_LEVEL__)
#define __INCLUDE_LEVEL__ 2
#endif
#include <systemd/sd-bus-protocol.h> // IWYU pragma: export
#include <systemd/sd-bus-vtable.h>   // IWYU pragma: export
#include <systemd/sd-bus.h>          // IWYU pragma: export
#include <systemd/sd-event.h>        // IWYU pragma: export

#endif // _FCITX_UTILS_DBUS_SD_BUS_WRAP_H_
