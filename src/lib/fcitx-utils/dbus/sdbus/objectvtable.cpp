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

#include "../objectvtable.h"
#include "../../log.h"
#include "bus_p.h"
#include "objectvtable_p.h"
#include <unordered_set>

namespace fcitx {
namespace dbus {

class ObjectVTablePrivate {
public:
    const std::string &vtableString(const std::string &str) {
        auto iter = stringPool_.find(str);
        if (iter == stringPool_.end()) {
            iter = stringPool_.insert(str).first;
        }
        return *iter;
    }

    bool hasVTable_ = false;
    std::vector<sd_bus_vtable> vtable_;
    std::unordered_set<std::string> stringPool_;
};

int SDMethodCallback(sd_bus_message *m, void *userdata, sd_bus_error *) {
    auto vtable = static_cast<ObjectVTableBase *>(userdata);
    if (!vtable) {
        return 0;
    }
    auto method = vtable->findMethod(sd_bus_message_get_member(m));
    if (!method) {
        return 0;
    }
    try {
        auto msg = MessagePrivate::fromSDBusMessage(m);
        auto wathcer = vtable->watch();
        vtable->setCurrentMessage(&msg);
        method->handler()(msg);
        if (wathcer.isValid()) {
            wathcer.get()->setCurrentMessage(nullptr);
        }
        return 1;
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_LOG(Error) << e.what();
        abort();
    }
    return 0;
}

int SDPropertyGetCallback(sd_bus *, const char *, const char *,
                          const char *property, sd_bus_message *reply,
                          void *userdata, sd_bus_error *) {
    auto vtable = static_cast<ObjectVTableBase *>(userdata);
    if (!vtable) {
        return 0;
    }
    auto prop = vtable->findProperty(property);
    if (!prop) {
        return 0;
    }
    try {
        auto msg = MessagePrivate::fromSDBusMessage(reply);
        prop->getMethod()(msg);
        return 1;
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_LOG(Error) << e.what();
        abort();
    }
    return 0;
}

int SDPropertySetCallback(sd_bus *, const char *, const char *,
                          const char *property, sd_bus_message *value,
                          void *userdata, sd_bus_error *) {
    auto vtable = static_cast<ObjectVTableBase *>(userdata);
    if (!vtable) {
        return 0;
    }
    auto prop = vtable->findProperty(property);
    if (!prop || !prop->writable()) {
        return 0;
    }
    try {
        auto msg = MessagePrivate::fromSDBusMessage(value);
        static_cast<ObjectVTableWritableProperty *>(prop)->setMethod()(msg);
        return 1;
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_LOG(Error) << e.what();
        abort();
    }
    return 0;
}

ObjectVTableBasePrivate::~ObjectVTableBasePrivate() {}

const sd_bus_vtable *
ObjectVTableBasePrivate::toSDBusVTable(ObjectVTableBase *q) {
    std::lock_guard<std::mutex> lock(q->privateDataMutexForType());
    auto p = q->privateDataForType();
    if (!p->hasVTable_) {
        std::vector<sd_bus_vtable> &result = p->vtable_;
        result.push_back(vtable_start());

        for (const auto &m : methods_) {
            auto method = m.second;
            result.push_back(vtable_method(
                p->vtableString(method->name()).c_str(),
                p->vtableString(method->signature()).c_str(),
                p->vtableString(method->ret()).c_str(), 0, SDMethodCallback));
        }

        for (const auto &s : sigs_) {
            auto sig = s.second;
            result.push_back(
                vtable_signal(p->vtableString(sig->name()).c_str(),
                              p->vtableString(sig->signature()).c_str()));
        }

        for (const auto &pr : properties_) {
            auto prop = pr.second;
            if (prop->writable()) {
                result.push_back(vtable_writable_property(
                    p->vtableString(prop->name()).c_str(),
                    p->vtableString(prop->signature()).c_str(),
                    SDPropertyGetCallback, SDPropertySetCallback));
            } else {
                result.push_back(
                    vtable_property(p->vtableString(prop->name()).c_str(),
                                    p->vtableString(prop->signature()).c_str(),
                                    SDPropertyGetCallback));
            }
        }

        result.push_back(vtable_end());
        p->hasVTable_ = true;
    }

    return p->vtable_.data();
}

ObjectVTableMethod::ObjectVTableMethod(ObjectVTableBase *vtable,
                                       const std::string &name,
                                       const std::string &signature,
                                       const std::string &ret,
                                       ObjectMethod handler)
    : name_(name), signature_(signature), ret_(ret), handler_(handler),
      vtable_(vtable) {
    vtable->addMethod(this);
}

ObjectVTableProperty::ObjectVTableProperty(ObjectVTableBase *vtable,
                                           const std::string &name,
                                           const std::string signature,
                                           PropertyGetMethod getMethod)
    : name_(name), signature_(signature), getMethod_(getMethod),
      writable_(false) {
    vtable->addProperty(this);
}

ObjectVTableWritableProperty::ObjectVTableWritableProperty(
    ObjectVTableBase *vtable, const std::string &name,
    const std::string signature, PropertyGetMethod getMethod,
    PropertySetMethod setMethod)
    : ObjectVTableProperty(vtable, name, signature, getMethod),
      setMethod_(setMethod) {
    writable_ = true;
}

ObjectVTableSignal::ObjectVTableSignal(ObjectVTableBase *vtable,
                                       const std::string &name,
                                       const std::string signature)
    : name_(name), signature_(signature), vtable_(vtable) {
    vtable->addSignal(this);
}

Message ObjectVTableSignal::createSignal() {
    return vtable_->bus()->createSignal(
        vtable_->path().c_str(), vtable_->interface().c_str(), name_.c_str());
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
    return d->slot_->bus_;
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
    d->slot_.reset(static_cast<SDVTableSlot *>(slot));
}
} // namespace dbus
} // namespace fcitx
