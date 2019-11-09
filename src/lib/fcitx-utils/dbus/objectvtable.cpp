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
#include "bus.h"
#include "objectvtable_p.h"

namespace fcitx {
namespace dbus {

ObjectVTableMethod::ObjectVTableMethod(ObjectVTableBase *vtable,
                                       const std::string &name,
                                       const std::string &signature,
                                       const std::string &ret,
                                       ObjectMethod handler)
    : d_ptr(std::make_unique<ObjectVTableMethodPrivate>(vtable, name, signature,
                                                        ret, handler)) {
    vtable->addMethod(this);
}

ObjectVTableMethod::~ObjectVTableMethod() {}

FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableMethod, std::string, name);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableMethod, std::string,
                                        signature);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableMethod, std::string, ret);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableMethod, ObjectMethod,
                                        handler);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableMethod, ObjectVTableBase *,
                                        vtable);

ObjectVTableSignal::ObjectVTableSignal(ObjectVTableBase *vtable,
                                       const std::string &name,
                                       const std::string signature)
    : d_ptr(std::make_unique<ObjectVTableSignalPrivate>(vtable, name,
                                                        signature)) {
    vtable->addSignal(this);
}

ObjectVTableSignal::~ObjectVTableSignal() {}

Message ObjectVTableSignal::createSignal() {
    FCITX_D();
    return d->vtable_->bus()->createSignal(d->vtable_->path().c_str(),
                                           d->vtable_->interface().c_str(),
                                           d->name_.c_str());
}

FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableSignal, std::string, name);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableSignal, std::string,
                                        signature);

ObjectVTableProperty::ObjectVTableProperty(ObjectVTableBase *vtable,
                                           const std::string &name,
                                           const std::string signature,
                                           PropertyGetMethod getMethod,
                                           PropertyOptions options)
    : d_ptr(std::make_unique<ObjectVTablePropertyPrivate>(name, signature,
                                                          getMethod, options)) {
    vtable->addProperty(this);
}

ObjectVTableProperty::ObjectVTableProperty(
    std::unique_ptr<ObjectVTablePropertyPrivate> d)
    : d_ptr(std::move(d)) {}

ObjectVTableProperty::~ObjectVTableProperty() {}

FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableProperty, std::string,
                                        name);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableProperty, std::string,
                                        signature);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableProperty, PropertyGetMethod,
                                        getMethod);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableProperty, bool, writable);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableProperty, PropertyOptions,
                                        options);

ObjectVTableWritableProperty::ObjectVTableWritableProperty(
    ObjectVTableBase *vtable, const std::string &name,
    const std::string signature, PropertyGetMethod getMethod,
    PropertySetMethod setMethod, PropertyOptions options)
    : ObjectVTableProperty(
          std::make_unique<ObjectVTableWritablePropertyPrivate>(
              name, signature, getMethod, setMethod, options)) {
    vtable->addProperty(this);
}

FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(ObjectVTableWritableProperty,
                                        PropertySetMethod, setMethod);

} // namespace dbus
} // namespace fcitx
