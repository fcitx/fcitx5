/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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

#include "../log.h"
#include "bus_p.h"
#include "message_p.h"
#include "objectvtable_p.h"

namespace fcitx {

namespace dbus {

Slot::~Slot() {}

class BusPrivate {
public:
    BusPrivate() : bus_(nullptr) {}

    ~BusPrivate() { sd_bus_flush_close_unref(bus_); }

    sd_bus *bus_;
    bool attached_ = false;
};

Bus::Bus(BusType type) : d_ptr(std::make_unique<BusPrivate>()) {
    decltype(&sd_bus_open) func;
    switch (type) {
    case BusType::Session:
        func = sd_bus_open_user;
        break;
    case BusType::System:
        func = sd_bus_open_system;
        break;
    default:
        func = sd_bus_open;
        break;
    }
    func(&d_ptr->bus_);
}

Bus::Bus(const std::string &address) : d_ptr(std::make_unique<BusPrivate>()) {
    if (sd_bus_new(&d_ptr->bus_) < 0) {
        goto fail;
    }

    if (sd_bus_set_address(d_ptr->bus_, address.c_str()) < 0) {
        goto fail;
    }

    if (sd_bus_set_bus_client(d_ptr->bus_, true) < 0) {
        goto fail;
    }

    if (sd_bus_start(d_ptr->bus_) < 0) {
        goto fail;
    }
    return;

fail:
    sd_bus_unref(d_ptr->bus_);
    d_ptr->bus_ = nullptr;
}

Bus::~Bus() {
    FCITX_D();
    if (d->attached_) {
        detachEventLoop();
    }
}

Bus::Bus(Bus &&other) noexcept : d_ptr(std::move(other.d_ptr)) {}

bool Bus::isOpen() const {
    FCITX_D();
    return d->bus_ && sd_bus_is_open(d->bus_) > 0;
}

Message Bus::createMethodCall(const char *destination, const char *path,
                              const char *interface, const char *member) {
    FCITX_D();
    Message msg;
    auto msgD = msg.d_func();
    if (sd_bus_message_new_method_call(d->bus_, &msgD->msg_, destination, path,
                                       interface, member) < 0) {
        msgD->type_ = MessageType::Invalid;
    } else {
        msgD->type_ = MessageType::MethodCall;
    }
    return msg;
}

Message Bus::createSignal(const char *path, const char *interface,
                          const char *member) {
    FCITX_D();
    Message msg;
    auto msgD = msg.d_func();
    int r = sd_bus_message_new_signal(d->bus_, &msgD->msg_, path, interface,
                                      member);
    if (r < 0) {
        msgD->type_ = MessageType::Invalid;
    } else {
        msgD->type_ = MessageType::Signal;
    }
    return msg;
}

void Bus::attachEventLoop(EventLoop *loop) {
    FCITX_D();
    sd_event *event = static_cast<sd_event *>(loop->nativeHandle());
    if (sd_bus_attach_event(d->bus_, event, 0) >= 0) {
        d->attached_ = true;
    }
}

void Bus::detachEventLoop() {
    FCITX_D();
    sd_bus_detach_event(d->bus_);
    d->attached_ = false;
}

int SDMessageCallback(sd_bus_message *m, void *userdata, sd_bus_error *) {
    auto slot = static_cast<SDSlot *>(userdata);
    if (!slot) {
        return 0;
    }
    try {
        auto result = slot->callback(MessagePrivate::fromSDBusMessage(m));
        return result ? 0 : 1;
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_LOG(Error) << e.what();
        abort();
    }
    return 1;
}

int SDEnumeratorCallback(sd_bus *, const char *prefix, void *userdata,
                         char ***ret_nodes, sd_bus_error *) {
    auto slot = static_cast<SDSubTreeSlot *>(userdata);
    if (!slot) {
        return 0;
    }
    try {
        auto result = slot->enumerator(prefix);
        auto ret =
            static_cast<char **>(malloc(sizeof(char *) * (result.size() + 1)));
        if (!ret) {
            return -ENOMEM;
        }
        for (size_t i = 0; i < result.size(); i++) {
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
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_LOG(Error) << e.what();
        abort();
    }
    return 1;
}

Slot *Bus::addMatch(const std::string &match, MessageCallback callback) {
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(callback);
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_match(d->bus_, &sdSlot, match.c_str(), SDMessageCallback,
                             slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot = sdSlot;

    return slot.release();
}

Slot *Bus::addFilter(MessageCallback callback) {
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(callback);
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_filter(d->bus_, &sdSlot, SDMessageCallback, slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot = sdSlot;

    return slot.release();
}

Slot *Bus::addObject(const std::string &path, MessageCallback callback) {
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(callback);
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_object(d->bus_, &sdSlot, path.c_str(), SDMessageCallback,
                              slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot = sdSlot;

    return slot.release();
}

bool Bus::addObjectVTable(const std::string &path, const std::string &interface,
                          ObjectVTableBase &vtable) {
    FCITX_D();
    auto slot = std::make_unique<SDVTableSlot>(this, path, interface);
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_object_vtable(
        d->bus_, &sdSlot, path.c_str(), interface.c_str(),
        vtable.d_func()->toSDBusVTable(&vtable), &vtable);
    if (r < 0) {
        return false;
    }

    slot->slot = sdSlot;

    vtable.setSlot(slot.release());
    return true;
}

Slot *Bus::addObjectSubTree(const std::string &path, MessageCallback callback,
                            EnumerateObjectCallback enumerator) {
    FCITX_D();
    auto slot = std::make_unique<SDSubTreeSlot>(callback, enumerator);
    sd_bus_slot *sdSlot, *sdEnumSlot;
    int r = sd_bus_add_node_enumerator(d->bus_, &sdSlot, path.c_str(),
                                       SDEnumeratorCallback, slot.get());
    if (r < 0) {
        return nullptr;
    }
    r = sd_bus_add_fallback(d->bus_, &sdEnumSlot, path.c_str(),
                            SDMessageCallback, slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot = sdSlot;
    slot->enumSlot = sdEnumSlot;

    return slot.release();
}

void *Bus::nativeHandle() const {
    FCITX_D();
    return d->bus_;
}

bool Bus::requestName(const std::string &name, Flags<RequestNameFlag> flags) {
    FCITX_D();
    int sd_flags = ((flags & RequestNameFlag::ReplaceExisting)
                        ? SD_BUS_NAME_REPLACE_EXISTING
                        : 0) |
                   ((flags & RequestNameFlag::AllowReplacement)
                        ? SD_BUS_NAME_ALLOW_REPLACEMENT
                        : 0) |
                   ((flags & RequestNameFlag::Queue) ? SD_BUS_NAME_QUEUE : 0);
    int r = sd_bus_request_name(d->bus_, name.c_str(), sd_flags);
    return r >= 0;
}

bool Bus::releaseName(const std::string &name) {
    FCITX_D();
    return sd_bus_release_name(d->bus_, name.c_str()) >= 0;
}

std::string Bus::serviceOwner(const std::string &name, uint64_t usec) {
    auto msg = createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                "org.freedesktop.DBus", "GetNameOwner");
    msg << name;
    auto reply = msg.call(usec);

    if (reply.type() == dbus::MessageType::Reply) {
        std::string ownerName;
        reply >> ownerName;
        return ownerName;
    }
    return {};
}

std::unique_ptr<Slot> Bus::serviceOwnerAsync(const std::string &name,
                                             uint64_t usec,
                                             MessageCallback callback) {
    auto msg = createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                "org.freedesktop.DBus", "GetNameOwner");
    msg << name;
    return msg.callAsync(usec, callback);
}

std::string Bus::uniqueName() {
    FCITX_D();
    const char *name = nullptr;
    if (sd_bus_get_unique_name(d->bus_, &name) < 0) {
        return {};
    }
    return name;
}

std::string Bus::address() {
    FCITX_D();
    const char *address = nullptr;
    if (sd_bus_get_address(d->bus_, &address) < 0) {
        return {};
    }
    return address;
}

void Bus::flush() {
    FCITX_D();
    sd_bus_flush(d->bus_);
}
}
}
