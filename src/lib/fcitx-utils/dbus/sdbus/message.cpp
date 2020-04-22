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

#include "../message.h"
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include <shared_mutex>
#include <stdexcept>
#include "../../misc_p.h"
#include "../../unixfd.h"
#include "../variant.h"
#include "bus_p.h"
#include "message_p.h"

namespace fcitx {

namespace dbus {

static char toSDBusType(Container::Type type) {
    char t = '\0';
    switch (type) {
    case Container::Type::Array:
        t = SD_BUS_TYPE_ARRAY;
        break;
    case Container::Type::DictEntry:
        t = SD_BUS_TYPE_DICT_ENTRY;
        break;
    case Container::Type::Struct:
        t = SD_BUS_TYPE_STRUCT;
        break;
    case Container::Type::Variant:
        t = SD_BUS_TYPE_VARIANT;
        break;
    default:
        throw std::runtime_error("invalid container type");
    }
    return t;
}

Message::Message() : d_ptr(std::make_unique<MessagePrivate>()) {}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(Message)

Message Message::createReply() const {
    FCITX_D();
    Message msg;
    auto msgD = msg.d_func();
    if (sd_bus_message_new_method_return(d->msg_, &msgD->msg_) < 0) {
        msgD->type_ = MessageType::Invalid;
    } else {
        msgD->type_ = MessageType::Reply;
    }
    return msg;
}

Message Message::createError(const char *name, const char *message) const {
    FCITX_D();
    Message msg;
    sd_bus_error error = SD_BUS_ERROR_MAKE_CONST(name, message);
    auto msgD = msg.d_func();
    int r = sd_bus_message_new_method_error(d->msg_, &msgD->msg_, &error);
    if (r < 0) {
        msgD->type_ = MessageType::Invalid;
    } else {
        msgD->type_ = MessageType::Error;
    }
    return msg;
}

MessageType Message::type() const {
    FCITX_D();
    return d->type_;
}

void Message::setDestination(const std::string &dest) {
    FCITX_D();
    if (d->msg_) {
        sd_bus_message_set_destination(d->msg_, dest.c_str());
    }
}

std::string Message::destination() const {
    FCITX_D();
    if (!d->msg_) {
        return {};
    }
    return sd_bus_message_get_destination(d->msg_);
}

std::string Message::sender() const {
    FCITX_D();
    if (!d->msg_) {
        return {};
    }
    return sd_bus_message_get_sender(d->msg_);
}

std::string Message::member() const {
    FCITX_D();
    if (!d->msg_) {
        return {};
    }
    auto member = sd_bus_message_get_member(d->msg_);
    return member ? member : "";
}

std::string Message::interface() const {
    FCITX_D();
    if (!d->msg_) {
        return {};
    }
    auto interface = sd_bus_message_get_interface(d->msg_);
    return interface ? interface : "";
}

std::string Message::signature() const {
    FCITX_D();
    return sd_bus_message_get_signature(d->msg_, true);
}

std::string Message::path() const {
    FCITX_D();
    auto path = sd_bus_message_get_path(d->msg_);
    return path ? path : "";
}

std::string Message::errorName() const {
    FCITX_D();
    if (d->msg_) {
        auto error = sd_bus_message_get_error(d->msg_);
        return error->name;
    } else {
        return d->error_;
    }
}

std::string Message::errorMessage() const {
    FCITX_D();
    if (d->msg_) {
        auto error = sd_bus_message_get_error(d->msg_);
        return error->message;
    } else {
        return d->message_;
    }
}

void *Message::nativeHandle() const {
    FCITX_D();
    return d->msg_;
}

Message Message::call(uint64_t timeout) {
    FCITX_D();
    ScopedSDBusError error;
    sd_bus_message *reply = nullptr;
    auto bus = sd_bus_message_get_bus(d->msg_);
    int r = sd_bus_call(bus, d->msg_, timeout, &error.error(), &reply);
    if (r < 0) {
        return MessagePrivate::fromSDError(error.error());
    }
    return MessagePrivate::fromSDBusMessage(reply, false);
}

std::unique_ptr<Slot> Message::callAsync(uint64_t timeout,
                                         MessageCallback callback) {
    FCITX_D();
    auto bus = sd_bus_message_get_bus(d->msg_);
    auto slot = std::make_unique<SDSlot>(callback);
    sd_bus_slot *sdSlot = nullptr;
    int r = sd_bus_call_async(bus, &sdSlot, d->msg_, SDMessageCallback,
                              slot.get(), timeout);
    if (r < 0) {
        return nullptr;
    }

    slot->slot_ = sdSlot;

    return slot;
}

Message::operator bool() const {
    FCITX_D();
    return d->lastError_ >= 0;
}

bool Message::end() const {
    FCITX_D();
    return sd_bus_message_at_end(d->msg_, 0) > 0;
}

void Message::resetError() {
    FCITX_D();
    d->lastError_ = 0;
}

void Message::rewind() {
    FCITX_D();
    sd_bus_message_rewind(d->msg_, true);
}

void Message::skip() {
    FCITX_D();
    if (!*this) {
        return;
    }
    d->lastError_ = sd_bus_message_skip(d->msg_, nullptr);
}

std::pair<char, std::string> Message::peekType() {
    FCITX_D();
    char container;
    const char *content;
    if (sd_bus_message_peek_type(d->msg_, &container, &content) >= 0) {
        return {container, content};
    }
    return {};
}

bool Message::send() {
    FCITX_D();
    auto bus = sd_bus_message_get_bus(d->msg_);
    return sd_bus_send(bus, d->msg_, 0) >= 0;
}

Message &Message::operator<<(bool b) {
    FCITX_D();
    int i = b ? 1 : 0;
    d->lastError_ =
        sd_bus_message_append_basic(d->msg_, SD_BUS_TYPE_BOOLEAN, &i);
    return *this;
}

Message &Message::operator>>(bool &b) {
    FCITX_D();
    int i = 0;
    d->lastError_ = sd_bus_message_read_basic(d->msg_, SD_BUS_TYPE_BOOLEAN, &i);
    b = i ? true : false;
    return *this;
}

#define _MARSHALL_FUNC(TYPE, TYPE2)                                            \
    Message &Message::operator<<(TYPE v) {                                     \
        if (!(*this)) {                                                        \
            return *this;                                                      \
        }                                                                      \
        FCITX_D();                                                             \
        d->lastError_ =                                                        \
            sd_bus_message_append_basic(d->msg_, SD_BUS_TYPE_##TYPE2, &v);     \
        return *this;                                                          \
    }                                                                          \
    Message &Message::operator>>(TYPE &v) {                                    \
        if (!(*this)) {                                                        \
            return *this;                                                      \
        }                                                                      \
        FCITX_D();                                                             \
        d->lastError_ =                                                        \
            sd_bus_message_read_basic(d->msg_, SD_BUS_TYPE_##TYPE2, &v);       \
        return *this;                                                          \
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
    *this << s.c_str();
    return *this;
}

Message &Message::operator<<(const char *s) {
    FCITX_D();
    if (!(*this)) {
        return *this;
    }
    d->lastError_ = sd_bus_message_append_basic(d->msg_, SD_BUS_TYPE_STRING, s);
    return *this;
}

Message &Message::operator>>(std::string &s) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    char *p = nullptr;
    int r = d->lastError_ =
        sd_bus_message_read_basic(d->msg_, SD_BUS_TYPE_STRING, &p);
    if (r < 0) {
    } else {
        s = p;
    }
    return *this;
}

Message &Message::operator<<(const ObjectPath &o) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    d->lastError_ = sd_bus_message_append_basic(
        d->msg_, SD_BUS_TYPE_OBJECT_PATH, o.path().c_str());
    return *this;
}

Message &Message::operator>>(ObjectPath &o) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    char *p = nullptr;
    int r = d->lastError_ =
        sd_bus_message_read_basic(d->msg_, SD_BUS_TYPE_OBJECT_PATH, &p);
    if (r < 0) {
    } else {
        o = ObjectPath(p);
    }
    return *this;
}

Message &Message::operator<<(const Signature &s) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    d->lastError_ = sd_bus_message_append_basic(d->msg_, SD_BUS_TYPE_SIGNATURE,
                                                s.sig().c_str());
    return *this;
}

Message &Message::operator>>(Signature &s) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    char *p = nullptr;
    int r = d->lastError_ =
        sd_bus_message_read_basic(d->msg_, SD_BUS_TYPE_SIGNATURE, &p);
    if (r < 0) {
    } else {
        s = Signature(p);
    }
    return *this;
}

Message &Message::operator<<(const UnixFD &fd) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    int f = fd.fd();
    d->lastError_ =
        sd_bus_message_append_basic(d->msg_, SD_BUS_TYPE_UNIX_FD, &f);
    return *this;
}

Message &Message::operator>>(UnixFD &fd) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    int f = -1;
    int r = d->lastError_ =
        sd_bus_message_read_basic(d->msg_, SD_BUS_TYPE_UNIX_FD, &f);
    if (r < 0) {
    } else {
        fd.give(f);
    }
    return *this;
}

Message &Message::operator<<(const Container &c) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();

    d->lastError_ = sd_bus_message_open_container(
        d->msg_, toSDBusType(c.type()), c.content().sig().c_str());
    return *this;
}

Message &Message::operator>>(const Container &c) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    d->lastError_ = sd_bus_message_enter_container(
        d->msg_, toSDBusType(c.type()), c.content().sig().c_str());
    return *this;
}

Message &Message::operator<<(const ContainerEnd &) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    d->lastError_ = sd_bus_message_close_container(d->msg_);
    return *this;
}

Message &Message::operator>>(const ContainerEnd &) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    d->lastError_ = sd_bus_message_exit_container(d->msg_);
    return *this;
}

Message &Message::operator<<(const Variant &v) {
    if (!(*this)) {
        return *this;
    }
    if (*this << Container(Container::Type::Variant,
                           Signature(v.signature()))) {
        v.writeToMessage(*this);
        if (!(*this)) {
            return *this;
        }
        if (*this) {
            *this << ContainerEnd();
        }
    }
    return *this;
}

Message &Message::operator>>(Variant &variant) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    auto type = peekType();
    if (type.first == 'v') {
        auto helper =
            VariantTypeRegistry::defaultRegistry().lookupType(type.second);
        if (helper) {
            if (*this >>
                Container(Container::Type::Variant, Signature(type.second))) {
                auto data = helper->copy(nullptr);
                helper->deserialize(*this, data.get());
                if (*this) {
                    variant.setRawData(data, helper);
                    *this >> ContainerEnd();
                }
            }
            return *this;
        }
    }
    d->lastError_ = sd_bus_message_skip(d->msg_, "v");
    return *this;
}
} // namespace dbus
} // namespace fcitx
