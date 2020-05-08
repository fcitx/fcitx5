/*
 * SPDX-FileCopyrightText: 2016-2019 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
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
