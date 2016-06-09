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

#include "dbus-message.h"
#include "dbus-message-p.h"
#include "dbus_p.h"
#include <fcntl.h>
#include <unistd.h>
#include <atomic>

namespace fcitx {

namespace dbus {

class UnixFDPrivate {
public:
    UnixFDPrivate() : m_fd(-1) {}
    ~UnixFDPrivate() {
        if (m_fd != -1) {
            int ret;
            do {
                ret = close(m_fd);
            } while (ret == -1 && errno == EINTR);
        }
    }
    std::atomic_int m_fd;
};

UnixFD::UnixFD(int fd) { set(fd); }

UnixFD::UnixFD(const UnixFD &other) : d(other.d) {}

UnixFD::UnixFD(UnixFD &&other) : d(std::move(other.d)) {}

UnixFD::~UnixFD() {}

UnixFD &UnixFD::operator=(UnixFD other) {
    using std::swap;
    swap(d, other.d);
    return *this;
}

bool UnixFD::isValid() const { return d && d->m_fd != -1; }

int UnixFD::fd() const { return d ? d->m_fd.load() : -1; }

void UnixFD::give(int fd) {
    if (fd == -1) {
        d.reset();
    } else {
        if (!d) {
            d = std::make_shared<UnixFDPrivate>();
        }

        d->m_fd = fd;
    }
}

void UnixFD::set(int fd) {
    if (fd == -1) {
        d.reset();
    } else {
        if (!d) {
            d = std::make_shared<UnixFDPrivate>();
        }

        int nfd = ::fcntl(fd, F_DUPFD_CLOEXEC, 0);
        if (nfd == -1) {
            // FIXME: throw exception
            return;
        }

        d->m_fd = nfd;
    }
}

int UnixFD::release() {
    int fd = d->m_fd.exchange(-1);
    d.reset();
    return fd;
}

Message::Message() : d_ptr(std::make_unique<MessagePrivate>()) {}

Message::~Message() {}

Message::Message(Message &&other) : d_ptr(std::move(other.d_ptr)) {}

Message Message::createReply() const {
    FCITX_D();
    Message msg;
    auto msgD = msg.d_func();
    if (sd_bus_message_new_method_return(d->msg, &msgD->msg) < 0) {
        msgD->type = MessageType::Invalid;
    } else {
        msgD->type = MessageType::Reply;
    }
    return msg;
}

Message Message::createError(const char *name, const char *message) const {
    FCITX_D();
    Message msg;
    sd_bus_error error = SD_BUS_ERROR_MAKE_CONST(name, message);
    auto msgD = msg.d_func();
    int r = sd_bus_message_new_method_error(d->msg, &msgD->msg, &error);
    if (r < 0) {
        msgD->type = MessageType::Invalid;
    } else {
        msgD->type = MessageType::Error;
    }
    return msg;
}

MessageType Message::type() const {
    FCITX_D();
    return d->type;
}

void Message::setDestination(const std::string &dest) {
    FCITX_D();
    if (d->msg) {
        sd_bus_message_set_destination(d->msg, dest.c_str());
    }
}

std::string Message::destination() const {
    FCITX_D();
    if (!d->msg) {
        return {};
    }
    return sd_bus_message_get_destination(d->msg);
}

std::string Message::signature() const {
    FCITX_D();
    return sd_bus_message_get_signature(d->msg, true);
}

void *Message::nativeHandle() const {
    FCITX_D();
    return d->msg;
}

Message Message::call(uint64_t timeout) {
    FCITX_D();
    ScopedSDBusError error;
    sd_bus_message *reply = nullptr;
    auto bus = sd_bus_message_get_bus(d->msg);
    int r = sd_bus_call(bus, d->msg, timeout, &error.error(), &reply);
    if (r < 0) {
        return createError(error.error().name, error.error().message);
    }
    return MessagePrivate::fromSDBusMessage(reply, false);
}

Slot *Message::callAsync(uint64_t timeout, MessageCallback callback) {
    FCITX_D();
    auto bus = sd_bus_message_get_bus(d->msg);
    auto slot = std::make_unique<SDSlot>(callback);
    sd_bus_slot *sdSlot = nullptr;
    int r = sd_bus_call_async(bus, &sdSlot, d->msg, SDMessageCallback, slot.get(), timeout);
    if (r < 0) {
        return nullptr;
    }

    slot->slot = sdSlot;

    return slot.release();
}

bool Message::send() {
    FCITX_D();
    auto bus = sd_bus_message_get_bus(d->msg);
    return sd_bus_send(bus, d->msg, 0) >= 0;
}

Message &Message::operator<<(bool b) {
    FCITX_D();
    int i = b ? 1 : 0;
    sd_bus_message_append_basic(d->msg, SD_BUS_TYPE_BOOLEAN, &i);
    return *this;
}

Message &Message::operator>>(bool &b) {
    FCITX_D();
    int i = 0;
    sd_bus_message_read_basic(d->msg, SD_BUS_TYPE_BOOLEAN, &i);
    b = i ? true : false;
    return *this;
}

#define _MARSHALL_FUNC(TYPE, TYPE2)                                                                                    \
    Message &Message::operator<<(TYPE v) {                                                                             \
        FCITX_D();                                                                                                     \
        sd_bus_message_append_basic(d->msg, SD_BUS_TYPE_##TYPE2, &v);                                                  \
        return *this;                                                                                                  \
    }                                                                                                                  \
    Message &Message::operator>>(TYPE &v) {                                                                            \
        FCITX_D();                                                                                                     \
        sd_bus_message_read_basic(d->msg, SD_BUS_TYPE_##TYPE2, &v);                                                    \
        return *this;                                                                                                  \
    }

_MARSHALL_FUNC(uint8_t, BYTE)
_MARSHALL_FUNC(int16_t, INT16)
_MARSHALL_FUNC(uint16_t, UINT16)
_MARSHALL_FUNC(int32_t, INT32)
_MARSHALL_FUNC(uint32_t, UINT32)
_MARSHALL_FUNC(int64_t, INT64)
_MARSHALL_FUNC(uint64_t, UINT64)
_MARSHALL_FUNC(double, DOUBLE)

Message &Message::operator<<(const std::string &s) {
    FCITX_D();
    sd_bus_message_append_basic(d->msg, SD_BUS_TYPE_STRING, s.c_str());
    return *this;
}

Message &Message::operator>>(std::string &s) {
    FCITX_D();
    char *p = nullptr;
    int r = sd_bus_message_read_basic(d->msg, SD_BUS_TYPE_STRING, &p);
    if (r < 0) {
    } else {
        s = p;
    }
    return *this;
}

Message &Message::operator<<(const ObjectPath &o) {
    FCITX_D();
    sd_bus_message_append_basic(d->msg, SD_BUS_TYPE_OBJECT_PATH, o.path().c_str());
    return *this;
}

Message &Message::operator>>(ObjectPath &o) {
    FCITX_D();
    char *p = nullptr;
    int r = sd_bus_message_read_basic(d->msg, SD_BUS_TYPE_OBJECT_PATH, &p);
    if (r < 0) {
    } else {
        o = ObjectPath(p);
    }
    return *this;
}

Message &Message::operator<<(const Signature &s) {
    FCITX_D();
    sd_bus_message_append_basic(d->msg, SD_BUS_TYPE_OBJECT_PATH, s.sig().c_str());
    return *this;
}

Message &Message::operator>>(Signature &s) {
    FCITX_D();
    char *p = nullptr;
    int r = sd_bus_message_read_basic(d->msg, SD_BUS_TYPE_OBJECT_PATH, &p);
    if (r < 0) {
    } else {
        s = Signature(p);
    }
    return *this;
}

Message &Message::operator<<(const UnixFD &fd) {
    FCITX_D();
    int f = fd.fd();
    sd_bus_message_append_basic(d->msg, SD_BUS_TYPE_UNIX_FD, &f);
    return *this;
}

Message &Message::operator>>(UnixFD &fd) {
    FCITX_D();
    int f = -1;
    int r = sd_bus_message_read_basic(d->msg, SD_BUS_TYPE_OBJECT_PATH, &f);
    if (r < 0) {
    } else {
        fd.give(f);
    }
    return *this;
}

Message &Message::operator<<(const Container &c) {
    FCITX_D();

    char t = '\0';
    switch (c.type()) {
    case Container::Type::Array:
        t = SD_BUS_TYPE_ARRAY;
        break;
    case Container::Type::DictEntry:
        t = SD_BUS_TYPE_STRUCT;
        break;
    case Container::Type::Struct:
        t = SD_BUS_TYPE_DICT_ENTRY;
        break;
    case Container::Type::Variant:
        t = SD_BUS_TYPE_VARIANT;
        break;
    default:
        throw std::runtime_error("invalid container type");
    }

    sd_bus_message_open_container(d->msg, t, c.content().sig().c_str());
    return *this;
}

Message &Message::operator>>(const Container &c) {
    FCITX_D();

    char t = '\0';
    switch (c.type()) {
    case Container::Type::Array:
        t = SD_BUS_TYPE_ARRAY;
        break;
    case Container::Type::DictEntry:
        t = SD_BUS_TYPE_STRUCT;
        break;
    case Container::Type::Struct:
        t = SD_BUS_TYPE_DICT_ENTRY;
        break;
    default:
        throw std::runtime_error("invalid container type");
    }

    sd_bus_message_enter_container(d->msg, t, c.content().sig().c_str());
    return *this;
}

Message &Message::operator<<(const ContainerEnd &) {
    FCITX_D();
    sd_bus_message_close_container(d->msg);
    return *this;
}

Message &Message::operator>>(const ContainerEnd &) {
    FCITX_D();
    sd_bus_message_exit_container(d->msg);
    return *this;
}
}
}
