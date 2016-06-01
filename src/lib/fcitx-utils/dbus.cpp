/*
 * Copyright (C) 2015~2015 by CSSlayer
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

#include <systemd/sd-bus.h>

#include "dbus.h"
#include "dbus-message-p.h"

namespace fcitx
{

namespace dbus
{

class ScopedSDBusError {
public:
    ScopedSDBusError() {
        m_error = SD_BUS_ERROR_NULL;
    }
    ~ScopedSDBusError() {
        sd_bus_error_free(&m_error);
    }

    sd_bus_error &error() {
        return m_error;
    }

private:
    sd_bus_error m_error;
};

Slot::~Slot() {
}

class SDSlot : public Slot
{
public:
    SDSlot(MessageCallback callback_) : callback(callback_), slot(nullptr) {
    }

    ~SDSlot() {
        sd_bus_slot_unref(slot);
    }

    MessageCallback callback;
    sd_bus_slot *slot;
};

class SDSubTreeSlot : public SDSlot
{
public:
    SDSubTreeSlot(MessageCallback callback_, EnumerateObjectCallback enumerator_) :
        SDSlot(callback_),
        enumerator(enumerator_),
        enumSlot(nullptr) {
    }

    ~SDSubTreeSlot() {
        sd_bus_slot_unref(enumSlot);
    }

    EnumerateObjectCallback enumerator;
    sd_bus_slot *enumSlot;
};

class BusPrivate
{
public:
    BusPrivate() : bus(nullptr) {
    }

    ~BusPrivate() {
        sd_bus_flush_close_unref(bus);
    }

    sd_bus *bus;
};

Bus::Bus(BusType type) : d_ptr(std::make_unique<BusPrivate>())
{
    decltype(&sd_bus_open) func;
    switch (type) {
        case BusType::Session:
            func = sd_bus_open_user;
        case BusType::System:
            func = sd_bus_open_system;
        default:
            func = sd_bus_open;
    }
    func(&d_ptr->bus);
}

Bus::Bus(const std::string &address) : d_ptr(std::make_unique<BusPrivate>())
{
    if (sd_bus_new(&d_ptr->bus) < 0) {
        goto fail;
    }

    if (sd_bus_set_address(d_ptr->bus, address.c_str()) < 0) {
        goto fail;
    }

    if (sd_bus_start(d_ptr->bus) < 0) {
        goto fail;
    }

fail:
    sd_bus_unref(d_ptr->bus);
    d_ptr->bus = nullptr;
}

Bus::~Bus()
{
}

Bus::Bus(Bus &&other) : d_ptr(std::make_unique<BusPrivate>()) {
    using std::swap;
    swap(d_ptr, other.d_ptr);
}

bool Bus::isOpen() const {
    FCITX_D();
    return d->bus && sd_bus_is_open(d->bus) > 0;
}

Message Bus::createMethodCall(const char *destination, const char *path, const char *interface, const char *member)
{
    FCITX_D();
    Message msg;
    auto msgD = msg.d_func();
    if (sd_bus_message_new_method_call(d->bus, &msgD->msg, destination, path, interface, member) < 0) {
        msgD->type = MessageType::Invalid;
    } else {
        msgD->type = MessageType::MethodCall;
    }
    return msg;
}

Message Bus::createSignal(const char *path, const char *interface, const char *member)
{
    FCITX_D();
    Message msg;
    auto msgD = msg.d_func();
    if (sd_bus_message_new_signal(d->bus, &msgD->msg, path, interface, member) < 0) {
        msgD->type = MessageType::Invalid;
    } else {
        msgD->type = MessageType::Signal;
    }
    return msg;
}

void Bus::attachEventLoop(EventLoop *loop)
{
    FCITX_D();
    sd_event *event = static_cast<sd_event *>(loop->nativeHandle());
    sd_bus_attach_event(d->bus, event, 0);
}

void Bus::detachEventLoop()
{
    FCITX_D();
    sd_bus_detach_event(d->bus);
}

int SDMessageCallback(sd_bus_message *m, void *userdata, sd_bus_error *)
{
    auto slot = static_cast<SDSlot*>(userdata);
    try {
        auto result = slot->callback(MessagePrivate::fromSDBusMessage(m));
        return result ? 0 : 1;
    } catch (...) {
        // some abnormal things threw
        abort();
    }
    return 1;
}

int SDEnumeratorCallback(sd_bus *, const char *prefix, void *userdata, char ***ret_nodes, sd_bus_error *)
{
    auto slot = static_cast<SDSubTreeSlot*>(userdata);
    try {
        auto result = slot->enumerator(prefix);
        auto ret = static_cast<char **>(malloc(sizeof(char *) * (result.size() + 1)));
        if (!ret) {
            return -ENOMEM;
        }
        for (size_t i = 0; i < result.size(); i ++) {
            ret[i] = strdup(result[i].c_str());
            if (!ret[i]) {
                int i = 0;
                while (ret[i]) {
                    free(ret[i]);
                    i++;
                }
                free(ret);
                return -ENOMEM;
            }
        }
        ret[result.size()] = nullptr;
        *ret_nodes = ret;
    } catch (...) {
        // some abnormal things threw
        abort();
    }
    return 1;
}


Slot *Bus::addMatch(const std::string &match, MessageCallback callback)
{
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(callback);
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_match(d->bus, &sdSlot, match.c_str(), SDMessageCallback, slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot = sdSlot;

    return slot.release();
}

Slot *Bus::addFilter(MessageCallback callback)
{
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(callback);
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_filter(d->bus, &sdSlot, SDMessageCallback, slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot = sdSlot;

    return slot.release();
}

Slot *Bus::addObject(const std::string &path, MessageCallback callback)
{
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(callback);
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_object(d->bus, &sdSlot, path.c_str(), SDMessageCallback, slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot = sdSlot;

    return slot.release();
}


Slot *Bus::addObjectSubTree(const std::string &path, MessageCallback callback, EnumerateObjectCallback enumerator)
{
    FCITX_D();
    auto slot = std::make_unique<SDSubTreeSlot>(callback, enumerator);
    sd_bus_slot *sdSlot, *sdEnumSlot;
    int r = sd_bus_add_node_enumerator(d->bus, &sdSlot, path.c_str(), SDEnumeratorCallback, slot.get());
    if (r < 0) {
        return nullptr;
    }
    r = sd_bus_add_fallback(d->bus, &sdEnumSlot, path.c_str(), SDMessageCallback, slot.get());

    slot->slot = sdSlot;
    slot->enumSlot = sdEnumSlot;

    return slot.release();
}


void *Bus::nativeHandle() const
{
    FCITX_D();
    return d->bus;
}

void Bus::send(Message msg) {
    FCITX_D();
    sd_bus_send(d->bus, msg.d_func()->msg, nullptr);
}

Message Bus::call(Message msg, uint64_t timeout) {
    FCITX_D();
    ScopedSDBusError error;
    sd_bus_message *reply = nullptr;
    int r = sd_bus_call(d->bus, msg.d_func()->msg, timeout, &error.error(), &reply);
    if (r < 0) {
        return msg.createError(error.error().name, error.error().message);
    }
    return MessagePrivate::fromSDBusMessage(reply, false);
}

Slot *Bus::callAsync(Message msg, uint64_t timeout, MessageCallback callback)
{
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(callback);
    sd_bus_slot *sdSlot = nullptr;
    int r = sd_bus_call_async(d->bus, &sdSlot, msg.d_func()->msg, SDMessageCallback, slot.get(), timeout);
    if (r < 0) {
        return nullptr;
    }

    slot->slot = sdSlot;

    return slot.release();
}

}

}
