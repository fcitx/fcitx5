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

#include "objectvtable.h"
#include "bus_p.h"
#include "objectvtable_p.h"

namespace fcitx {
namespace dbus {

int SDMethodCallback(sd_bus_message *m, void *userdata, sd_bus_error *) {
    try {
        auto method = static_cast<ObjectVTableMethod *>(userdata);
        auto msg = MessagePrivate::fromSDBusMessage(m);
        auto wathcer = method->vtable()->watch();
        method->vtable()->setCurrentMessage(&msg);
        method->handler()(msg);
        if (wathcer.isValid()) {
            wathcer.get()->setCurrentMessage(nullptr);
        }
        return 1;
    } catch (...) {
        // some abnormal things threw
        abort();
    }
    return 0;
}

ObjectVTablePrivate::~ObjectVTablePrivate() {}

std::vector<sd_bus_vtable> ObjectVTablePrivate::toSDBusVTable() {
    std::vector<sd_bus_vtable> result;
    result.push_back(vtable_start());

    for (auto method : methods_) {
        auto offset =
            reinterpret_cast<char *>(method) - reinterpret_cast<char *>(q_ptr);
        result.push_back(vtable_method(
            vtableString(method->name()).c_str(),
            vtableString(method->signature()).c_str(),
            vtableString(method->ret()).c_str(), offset, SDMethodCallback));
    }

    result.push_back(vtable_end());
    return result;
}

ObjectVTableMethod::ObjectVTableMethod(ObjectVTable *vtable,
                                       const std::string &name,
                                       const std::string &signature,
                                       const std::string &ret,
                                       ObjectMethod handler)
    : name_(name), signature_(signature), ret_(ret), handler_(handler),
      vtable_(vtable) {
    vtable->addMethod(this);
}

ObjectVTableProperty::ObjectVTableProperty(ObjectVTable *vtable,
                                           const std::string &name,
                                           const std::string signature,
                                           PropertyGetMethod getMethod)
    : name_(name), signature_(signature), getMethod_(getMethod),
      writable_(false) {
    vtable->addProperty(this);
}

ObjectVTableWritableProperty::ObjectVTableWritableProperty(
    ObjectVTable *vtable, const std::string &name, const std::string signature,
    PropertyGetMethod getMethod, PropertySetMethod setMethod)
    : ObjectVTableProperty(vtable, name, signature, getMethod),
      setMethod_(setMethod) {
    writable_ = true;
}

ObjectVTableSignal::ObjectVTableSignal(ObjectVTable *vtable,
                                       const std::string &name,
                                       const std::string signature)
    : name_(name), signature_(signature), vtable_(vtable) {
    vtable->addSignal(this);
}

Message ObjectVTableSignal::createSignal() {
    return vtable_->bus()->createSignal(
        vtable_->path().c_str(), vtable_->interface().c_str(), name_.c_str());
}

ObjectVTable::ObjectVTable()
    : d_ptr(std::make_unique<ObjectVTablePrivate>(this)) {}

ObjectVTable::~ObjectVTable() {}

void ObjectVTable::addMethod(ObjectVTableMethod *method) {
    FCITX_D();
    d->methods_.push_back(method);
}

void ObjectVTable::addProperty(ObjectVTableProperty *property) {
    FCITX_D();
    d->properties_.push_back(property);
}

void ObjectVTable::addSignal(ObjectVTableSignal *signal) {
    FCITX_D();
    d->sigs_.push_back(signal);
}

void ObjectVTable::releaseSlot() { setSlot(nullptr); }

Bus *ObjectVTable::bus() {
    FCITX_D();
    return d->slot_->bus;
}

const std::string &ObjectVTable::path() const {
    FCITX_D();
    return d->slot_->path;
}

const std::string &ObjectVTable::interface() const {
    FCITX_D();
    return d->slot_->interface;
}

Message *ObjectVTable::currentMessage() const {
    FCITX_D();
    return d->msg_;
}

void ObjectVTable::setCurrentMessage(Message *msg) {
    FCITX_D();
    d->msg_ = msg;
}

void ObjectVTable::setSlot(Slot *slot) {
    FCITX_D();
    d->slot_.reset(static_cast<SDVTableSlot *>(slot));
}
}
}
