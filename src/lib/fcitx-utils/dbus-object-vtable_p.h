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
#ifndef _FCITX_UTILS_DBUS_OBJECT_VTABLE_P_H_
#define _FCITX_UTILS_DBUS_OBJECT_VTABLE_P_H_

#include <systemd/sd-bus.h>
#include <vector>
#include "dbus-object-vtable.h"
#include "dbus-object-vtable-wrapper.h"
#include "dbus-message-p.h"

namespace fcitx
{
namespace dbus
{


class ObjectVTablePrivate
{
public:
    ObjectVTablePrivate(ObjectVTable *q) : q_ptr(q) { }
    ~ObjectVTablePrivate();

    std::vector<sd_bus_vtable> toSDBusVTable() const;

    ObjectVTable *q_ptr;
    FCITX_DECLARE_PUBLIC(ObjectVTable);

    std::vector<ObjectVTableMethod *> methods;
    std::vector<ObjectVTableProperty *> properties;
    std::vector<ObjectVTableSignal *> sigs;
    std::unique_ptr<Slot> slot;
};

}
}

#endif // _FCITX_UTILS_DBUS_OBJECT_VTABLE_P_H_
