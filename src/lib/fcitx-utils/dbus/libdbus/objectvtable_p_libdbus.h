/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_OBJECTVTABLE_P_H_
#define _FCITX_UTILS_DBUS_OBJECTVTABLE_P_H_

#include <map>
#include <vector>
#include "../objectvtable.h"
#include "message_p.h"

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
