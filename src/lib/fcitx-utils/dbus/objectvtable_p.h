/*
 * SPDX-FileCopyrightText: 2016-2019 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
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
