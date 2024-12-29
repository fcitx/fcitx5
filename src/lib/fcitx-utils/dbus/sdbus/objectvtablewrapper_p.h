/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_OBJECTVTABLEWRAPPER_P_H_
#define _FCITX_UTILS_DBUS_OBJECTVTABLEWRAPPER_P_H_

#include <stddef.h> // NOLINT(modernize-deprecated-headers)
#include <stdint.h> // NOLINT(modernize-deprecated-headers)
#include "sd-bus-wrap.h"

#ifdef __cplusplus
extern "C" {
#endif

sd_bus_vtable vtable_start();
sd_bus_vtable vtable_method(const char *member, const char *signature,
                            const char *ret, size_t offset,
                            sd_bus_message_handler_t handler);
sd_bus_vtable vtable_signal(const char *member, const char *signature);
sd_bus_vtable vtable_property(const char *member, const char *signature,
                              sd_bus_property_get_t getter, uint32_t flags);
sd_bus_vtable vtable_writable_property(const char *member,
                                       const char *signature,
                                       sd_bus_property_get_t getter,
                                       sd_bus_property_set_t setter,
                                       uint32_t flags);
sd_bus_vtable vtable_end();

#ifdef __cplusplus
}
#endif

#endif // _FCITX_UTILS_DBUS_OBJECTVTABLEWRAPPER_P_H_
