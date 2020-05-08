/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "objectvtablewrapper_p.h"

sd_bus_vtable vtable_start() {
    sd_bus_vtable result = SD_BUS_VTABLE_START(0);
    return result;
}

sd_bus_vtable vtable_method(const char *member, const char *signature,
                            const char *ret, size_t offset,
                            sd_bus_message_handler_t handler) {
    sd_bus_vtable result =
        SD_BUS_METHOD_WITH_OFFSET(member, signature, ret, handler, offset, 0);
    return result;
}

sd_bus_vtable vtable_signal(const char *member, const char *signature) {
    sd_bus_vtable result = SD_BUS_SIGNAL(member, signature, 0);
    return result;
}

sd_bus_vtable vtable_property(const char *member, const char *signature,
                              sd_bus_property_get_t getter, uint32_t flags) {
    sd_bus_vtable result = SD_BUS_PROPERTY(member, signature, getter, 0, flags);
    return result;
}

sd_bus_vtable vtable_writable_property(const char *member,
                                       const char *signature,
                                       sd_bus_property_get_t getter,
                                       sd_bus_property_set_t setter,
                                       uint32_t flags) {
    sd_bus_vtable result =
        SD_BUS_WRITABLE_PROPERTY(member, signature, getter, setter, 0, flags);
    return result;
}

sd_bus_vtable vtable_end() {
    sd_bus_vtable result = SD_BUS_VTABLE_END;
    return result;
}
