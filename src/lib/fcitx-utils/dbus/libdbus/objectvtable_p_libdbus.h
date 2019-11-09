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
#ifndef _FCITX_UTILS_DBUS_OBJECTVTABLE_P_H_
#define _FCITX_UTILS_DBUS_OBJECTVTABLE_P_H_

#include "../objectvtable.h"
#include "message_p.h"
#include <map>
#include <vector>

namespace fcitx {
namespace dbus {
class DBusObjectVTableSlot;

class ObjectVTableBasePrivate {
public:
    ObjectVTableBasePrivate() {}
    ~ObjectVTableBasePrivate();

    const std::string &getXml(ObjectVTableBase *q);

    std::map<std::string, ObjectVTableMethod *> methods_;
    std::map<std::string, ObjectVTableProperty *> properties_;
    std::map<std::string, ObjectVTableSignal *> sigs_;
    std::unique_ptr<DBusObjectVTableSlot> slot_;
    Message *msg_ = nullptr;
};
} // namespace dbus
} // namespace fcitx

#endif // _FCITX_UTILS_DBUS_OBJECTVTABLE_P_H_
