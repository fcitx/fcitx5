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

#include <unordered_set>
#include "../../log.h"
#include "../../stringutils.h"
#include "../objectvtable.h"
#include "../utils_p.h"
#include "bus_p.h"
#include "objectvtable_p_libdbus.h"

namespace fcitx {
namespace dbus {

class ObjectVTablePrivate {
public:
    bool hasXml_ = false;
    std::string xml_;
};

ObjectVTableBasePrivate::~ObjectVTableBasePrivate() {}

const std::string &ObjectVTableBasePrivate::getXml(ObjectVTableBase *q) {
    std::lock_guard<std::mutex> lock(q->privateDataMutexForType());
    auto p = q->privateDataForType();
    if (!p->hasXml_) {
        p->xml_.clear();

        for (const auto &m : methods_) {
            auto method = m.second;
            p->xml_ +=
                stringutils::concat("<method name=\"", method->name(), "\">");
            for (auto &type : splitDBusSignature(method->signature())) {
                p->xml_ += stringutils::concat("<arg direction=\"in\" type=\"",
                                               type, "\"/>");
            }
            for (auto &type : splitDBusSignature(method->ret())) {
                p->xml_ += stringutils::concat("<arg direction=\"in\" type=\"",
                                               type, "\"/>");
            }
            p->xml_ += "</method>";
        }

        for (const auto &s : sigs_) {
            auto sig = s.second;
            p->xml_ +=
                stringutils::concat("<signal name=\"", sig->name(), "\">");
            for (auto &type : splitDBusSignature(sig->signature())) {
                p->xml_ += stringutils::concat("<arg direction=\"in\" type=\"",
                                               type, "\"/>");
            }
            p->xml_ += "</signal>";
        }

        for (const auto &pr : properties_) {
            auto prop = pr.second;
            if (prop->writable()) {
                p->xml_ += stringutils::concat(
                    "<property access=\"readwrite\" type=\"", prop->signature(),
                    "\" name=\"", prop->name(), "\">");
            } else {
                p->xml_ += stringutils::concat(
                    "<property access=\"read\" type=\"", prop->signature(),
                    "\" name=\"", prop->name(), "\">");
            }
            p->xml_ += "</property>";
        }
        p->hasXml_ = true;
    }

    return p->xml_;
}

ObjectVTableBase::ObjectVTableBase()
    : d_ptr(std::make_unique<ObjectVTableBasePrivate>()) {}

ObjectVTableBase::~ObjectVTableBase() {}

void ObjectVTableBase::addMethod(ObjectVTableMethod *method) {
    FCITX_D();
    d->methods_[method->name()] = method;
}

void ObjectVTableBase::addProperty(ObjectVTableProperty *property) {
    FCITX_D();
    d->properties_[property->name()] = property;
}

void ObjectVTableBase::addSignal(ObjectVTableSignal *signal) {
    FCITX_D();
    d->sigs_[signal->name()] = signal;
}

ObjectVTableMethod *ObjectVTableBase::findMethod(const std::string &name) {
    FCITX_D();
    auto iter = d->methods_.find(name);
    if (iter == d->methods_.end()) {
        return nullptr;
    }
    return iter->second;
}

ObjectVTableProperty *ObjectVTableBase::findProperty(const std::string &name) {
    FCITX_D();
    auto iter = d->properties_.find(name);
    if (iter == d->properties_.end()) {
        return nullptr;
    }
    return iter->second;
}

void ObjectVTableBase::releaseSlot() { setSlot(nullptr); }

Bus *ObjectVTableBase::bus() {
    FCITX_D();
    if (d->slot_) {
        if (auto bus = d->slot_->bus_.get()) {
            return bus->bus_;
        }
    }
    return nullptr;
}

bool ObjectVTableBase::isRegistered() const {
    FCITX_D();
    return !!d->slot_;
}

const std::string &ObjectVTableBase::path() const {
    FCITX_D();
    return d->slot_->path_;
}

const std::string &ObjectVTableBase::interface() const {
    FCITX_D();
    return d->slot_->interface_;
}

Message *ObjectVTableBase::currentMessage() const {
    FCITX_D();
    return d->msg_;
}

void ObjectVTableBase::setCurrentMessage(Message *msg) {
    FCITX_D();
    d->msg_ = msg;
}

std::shared_ptr<ObjectVTablePrivate> ObjectVTableBase::newSharedPrivateData() {
    return std::make_shared<ObjectVTablePrivate>();
}

void ObjectVTableBase::setSlot(Slot *slot) {
    FCITX_D();
    d->slot_.reset(static_cast<DBusObjectVTableSlot *>(slot));
}
} // namespace dbus
} // namespace fcitx
