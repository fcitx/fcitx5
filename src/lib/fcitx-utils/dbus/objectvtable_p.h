//
// Copyright (C) 2016~2019 by CSSlayer
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

#include "objectvtable.h"

namespace fcitx {
namespace dbus {

class ObjectVTableMethodPrivate {
public:
    ObjectVTableMethodPrivate(ObjectVTableBase *vtable, const std::string &name,
                              const std::string &signature,
                              const std::string &ret, ObjectMethod handler)
        : name_(name), signature_(signature), ret_(ret), handler_(handler),
          vtable_(vtable) {}

    const std::string name_;
    const std::string signature_;
    const std::string ret_;
    ObjectMethod handler_;
    ObjectVTableBase *vtable_;
};

class ObjectVTableSignalPrivate {
public:
    ObjectVTableSignalPrivate(ObjectVTableBase *vtable, const std::string &name,
                              const std::string signature)
        : name_(name), signature_(signature), vtable_(vtable) {}
    const std::string name_;
    const std::string signature_;
    ObjectVTableBase *vtable_;
};

class ObjectVTablePropertyPrivate {
public:
    ObjectVTablePropertyPrivate(const std::string &name,
                                const std::string signature,
                                PropertyGetMethod getMethod,
                                PropertyOptions options)
        : name_(name), signature_(signature), getMethod_(getMethod),
          writable_(false), options_(options) {}

    const std::string name_;
    const std::string signature_;
    PropertyGetMethod getMethod_;
    bool writable_;
    PropertyOptions options_;
};

class ObjectVTableWritablePropertyPrivate : public ObjectVTablePropertyPrivate {
public:
    ObjectVTableWritablePropertyPrivate(const std::string &name,
                                        const std::string signature,
                                        PropertyGetMethod getMethod,
                                        PropertySetMethod setMethod,
                                        PropertyOptions options)
        : ObjectVTablePropertyPrivate(name, signature, getMethod, options),
          setMethod_(setMethod) {
        writable_ = true;
    }

    PropertySetMethod setMethod_;
};

} // namespace dbus
} // namespace fcitx

#endif // _FCITX_UTILS_DBUS_OBJECTVTABLE_P_H_
