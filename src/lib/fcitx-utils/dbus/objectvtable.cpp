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

#include "dbus/objectvtable.h"
#include "dbus/bus_p.h"
#include "dbus/objectvtable_p.h"

namespace fcitx {
namespace dbus {

int SDMethodCallback(sd_bus_message *m, void *userdata, sd_bus_error *) {
    try {
        auto method = static_cast<ObjectVTableMethod *>(userdata);
        method->handler()(MessagePrivate::fromSDBusMessage(m));
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

    for (auto method : methods) {
        auto offset = reinterpret_cast<char *>(method) - reinterpret_cast<char *>(q_ptr);
        result.push_back(vtable_method(vtableString(method->name()).c_str(), vtableString(method->signature()).c_str(),
                                       vtableString(method->ret()).c_str(), offset, SDMethodCallback));
    }

    result.push_back(vtable_end());
    return result;
}

ObjectVTableMethod::ObjectVTableMethod(ObjectVTable *vtable, const std::string &name, const std::string &signature,
                                       const std::string &ret, ObjectMethod handler)
    : m_name(name), m_signature(signature), m_ret(ret), m_handler(handler) {
    vtable->addMethod(this);
}

ObjectVTableProperty::ObjectVTableProperty(ObjectVTable *vtable, const std::string &name, const std::string signature,
                                           PropertyGetMethod getMethod)
    : m_name(name), m_signature(signature), m_getMethod(getMethod), m_writable(false) {
    vtable->addProperty(this);
}

ObjectVTableWritableProperty::ObjectVTableWritableProperty(ObjectVTable *vtable, const std::string &name,
                                                           const std::string signature, PropertyGetMethod getMethod,
                                                           PropertySetMethod setMethod)
    : ObjectVTableProperty(vtable, name, signature, getMethod), m_setMethod(setMethod) {
    m_writable = true;
}

ObjectVTableSignal::ObjectVTableSignal(ObjectVTable *vtable, const std::string &name, const std::string signature)
    : m_name(name), m_signature(signature), m_vtable(vtable) {
    vtable->addSignal(this);
}

Message ObjectVTableSignal::createSignal() {
    return m_vtable->bus()->createSignal(m_vtable->path().c_str(), m_vtable->interface().c_str(), m_name.c_str());
}

ObjectVTable::ObjectVTable() : d_ptr(std::make_unique<ObjectVTablePrivate>(this)) {}

ObjectVTable::~ObjectVTable() {}

void ObjectVTable::addMethod(ObjectVTableMethod *method) {
    FCITX_D();
    d->methods.push_back(method);
}

void ObjectVTable::addProperty(ObjectVTableProperty *property) {
    FCITX_D();
    d->properties.push_back(property);
}

void ObjectVTable::addSignal(ObjectVTableSignal *signal) {
    FCITX_D();
    d->sigs.push_back(signal);
}

void ObjectVTable::releaseSlot() { setSlot(nullptr); }

Bus *ObjectVTable::bus() {
    FCITX_D();
    return d->slot->bus;
}

const std::string &ObjectVTable::path() const {
    FCITX_D();
    return d->slot->path;
}

const std::string &ObjectVTable::interface() const {
    FCITX_D();
    return d->slot->interface;
}

Message *ObjectVTable::currentMessage() const {
    FCITX_D();
    return d->msg;
}

void ObjectVTable::setCurrentMessage(Message *msg) {
    FCITX_D();
    d->msg = msg;
}

void ObjectVTable::setSlot(Slot *slot) {
    FCITX_D();
    d->slot.reset(static_cast<SDVTableSlot *>(slot));
}
}
}
