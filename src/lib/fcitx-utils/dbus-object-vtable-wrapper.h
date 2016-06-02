/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_UTILS_DBUS_OBJECT_VTABLE_WRAPPER_H_
#define _FCITX_UTILS_DBUS_OBJECT_VTABLE_WRAPPER_H_

#include <systemd/sd-bus.h>

#ifdef __cplusplus
extern "C"
{
#endif

sd_bus_vtable vtable_start();
sd_bus_vtable vtable_method(const char *member, const char *signature, const char *ret, size_t offset, sd_bus_message_handler_t handler);
sd_bus_vtable vtable_signal(const char *member, const char *signature);
sd_bus_vtable vtable_property(const char *member, const char *signature, sd_bus_property_get_t getter);
sd_bus_vtable vtable_writable_property(const char *member, const char *signature, sd_bus_property_get_t getter, sd_bus_property_set_t setter);
sd_bus_vtable vtable_end();

#ifdef __cplusplus
}
#endif

#endif // _FCITX_UTILS_DBUS_OBJECT_VTABLE_WRAPPER_H_
