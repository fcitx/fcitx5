/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <stdexcept>
#include "../../log.h"
#include "bus_p.h"
#include "message_p.h"
#include "objectvtable_p_sdbus.h"

namespace fcitx::dbus {

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
    if (func(&d_ptr->bus_) < 0) {
        sd_bus_unref(d_ptr->bus_);
        d_ptr->bus_ = nullptr;
        throw std::runtime_error("Failed to create dbus connection");
    }
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
    throw std::runtime_error("Failed to create dbus connection");
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
    auto *msgD = msg.d_func();
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
    auto *msgD = msg.d_func();
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
    auto *slot = static_cast<SDSlot *>(userdata);
    if (!slot) {
        return 0;
    }
    try {
        auto msg = MessagePrivate::fromSDBusMessage(m);
        auto result = slot->callback_(msg);
        return result ? 1 : 0;
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_ERROR() << e.what();
        abort();
    }
    return 1;
}

std::unique_ptr<Slot> Bus::addMatch(const MatchRule &rule,
                                    MessageCallback callback) {
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(std::move(callback));
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_match(d->bus_, &sdSlot, rule.rule().c_str(),
                             SDMessageCallback, slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot_ = sdSlot;

    return slot;
}

std::unique_ptr<Slot> Bus::addFilter(MessageCallback callback) {
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(std::move(callback));
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_filter(d->bus_, &sdSlot, SDMessageCallback, slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot_ = sdSlot;

    return slot;
}

std::unique_ptr<Slot> Bus::addObject(const std::string &path,
                                     MessageCallback callback) {
    FCITX_D();
    auto slot = std::make_unique<SDSlot>(std::move(callback));
    sd_bus_slot *sdSlot;
    int r = sd_bus_add_object(d->bus_, &sdSlot, path.c_str(), SDMessageCallback,
                              slot.get());
    if (r < 0) {
        return nullptr;
    }

    slot->slot_ = sdSlot;

    return slot;
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

    slot->slot_ = sdSlot;

    vtable.setSlot(slot.release());
    return true;
}

const char *Bus::impl() { return "sdbus"; }

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
    return r >= 0 || r == -EALREADY;
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
    return msg.callAsync(usec, std::move(callback));
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
} // namespace fcitx::dbus
