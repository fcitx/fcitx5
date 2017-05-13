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
#ifndef _FCITX_UTILS_DBUS_OBJECTVTABLE_P_H_
#define _FCITX_UTILS_DBUS_OBJECTVTABLE_P_H_

#include <fcitx-utils/dbus/message_p.h>
#include <fcitx-utils/dbus/objectvtable.h>
#include <fcitx-utils/dbus/objectvtablewrapper_p.h>
#include <unordered_set>
#include <vector>

namespace fcitx {
namespace dbus {
class SDVTableSlot;

class ObjectVTableBasePrivate {
public:
    ObjectVTableBasePrivate(ObjectVTableBase *q) : q_ptr(q) {}
    ~ObjectVTableBasePrivate();

    const sd_bus_vtable *toSDBusVTable();

    ObjectVTableBase *q_ptr;
    FCITX_DECLARE_PUBLIC(ObjectVTableBase);
    std::vector<ObjectVTableMethod *> methods_;
    std::vector<ObjectVTableProperty *> properties_;
    std::vector<ObjectVTableSignal *> sigs_;
    std::unique_ptr<SDVTableSlot> slot_;
    Message *msg_ = nullptr;
};
}
}

#endif // _FCITX_UTILS_DBUS_OBJECTVTABLE_P_H_
