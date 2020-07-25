/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "../message.h"
#include <fcntl.h>
#include <unistd.h>
#include <atomic>
#include <stdexcept>
#include "../../misc_p.h"
#include "../../unixfd.h"
#include "../variant.h"
#include "bus_p.h"
#include "message_p.h"

namespace fcitx {

namespace dbus {

static char toDBusType(Container::Type type) {
    char t = '\0';
    switch (type) {
    case Container::Type::Array:
        t = DBUS_TYPE_ARRAY;
        break;
    case Container::Type::DictEntry:
        t = DBUS_TYPE_DICT_ENTRY;
        break;
    case Container::Type::Struct:
        t = DBUS_TYPE_STRUCT;
        break;
    case Container::Type::Variant:
        t = DBUS_TYPE_VARIANT;
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
    auto dmsg = dbus_message_new_method_return(d->msg());
    if (!dmsg) {
        return {};
    }
    return MessagePrivate::fromDBusMessage(d->bus_, dmsg, true, false);
}

Message Message::createError(const char *name, const char *message) const {
    FCITX_D();
    auto dmsg = dbus_message_new_error(d->msg(), name, message);
    if (!dmsg) {
        return {};
    }
    return MessagePrivate::fromDBusMessage(d->bus_, dmsg, false, false);
}

MessageType Message::type() const {
    FCITX_D();
    return d->type_;
}

void Message::setDestination(const std::string &dest) {
    FCITX_D();
    if (d->msg()) {
        dbus_message_set_destination(d->msg(), dest.c_str());
    }
}

std::string Message::destination() const {
    FCITX_D();
    if (!d->msg()) {
        return {};
    }
    return dbus_message_get_destination(d->msg());
}

std::string Message::sender() const {
    FCITX_D();
    if (!d->msg()) {
        return {};
    }
    auto sender = dbus_message_get_sender(d->msg());
    return sender ? sender : "";
}

std::string Message::member() const {
    FCITX_D();
    if (!d->msg()) {
        return {};
    }
    auto member = dbus_message_get_member(d->msg());
    return member ? member : "";
}

std::string Message::interface() const {
    FCITX_D();
    if (!d->msg()) {
        return {};
    }
    auto interface = dbus_message_get_interface(d->msg());
    return interface ? interface : "";
}

std::string Message::signature() const {
    FCITX_D();
    if (!d->msg()) {
        return {};
    }
    auto signature = dbus_message_get_signature(d->msg());
    return signature ? signature : "";
}

std::string Message::path() const {
    FCITX_D();
    auto path = dbus_message_get_path(d->msg());
    return path ? path : "";
}

std::string Message::errorName() const {
    FCITX_D();
    if (d->msg()) {
        auto err = dbus_message_get_error_name(d->msg());
        return err ? err : "";
    } else {
        return d->error_;
    }
}

std::string Message::errorMessage() const {
    FCITX_D();
    if (d->msg()) {
        char *message = nullptr;
        if (dbus_message_get_args(d->msg(), nullptr, DBUS_TYPE_STRING, &message,
                                  DBUS_TYPE_INVALID)) {
            return message;
        }
        return "";
    } else {
        return d->message_;
    }
}

void *Message::nativeHandle() const {
    FCITX_D();
    return d->msg();
}

int convertTimeout(uint64_t timeout) {
    int milliTimout = timeout / 1000;
    if (timeout > 0 && milliTimout == 0) {
        milliTimout = 1;
    } else if (timeout == 0) {
        milliTimout = DBUS_TIMEOUT_USE_DEFAULT;
    }
    return milliTimout;
}

Message Message::call(uint64_t timeout) {
    FCITX_D();
    ScopedDBusError error;
    auto bus = d->bus_.get();
    if (!bus) {
        return Message();
    }
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        bus->conn_.get(), d->msg(), convertTimeout(timeout), &error.error());
    if (!reply) {
        return MessagePrivate::fromDBusError(error.error());
    }
    return MessagePrivate::fromDBusMessage(bus->watch(), reply, false, false);
}

void DBusPendingCallNotifyCallback(DBusPendingCall *reply, void *userdata) {
    auto slot = static_cast<DBusAsyncCallSlot *>(userdata);
    if (!slot) {
        return;
    }

    auto msg = MessagePrivate::fromDBusMessage(
        slot->bus_, dbus_pending_call_steal_reply(reply), false, false);
    slot->callback_(msg);
}

std::unique_ptr<Slot> Message::callAsync(uint64_t timeout,
                                         MessageCallback callback) {
    FCITX_D();
    auto bus = d->bus_.get();
    if (!bus) {
        return nullptr;
    }
    auto slot = std::make_unique<DBusAsyncCallSlot>(callback);
    DBusPendingCall *call = nullptr;
    if (!dbus_connection_send_with_reply(bus->conn_.get(), d->msg(), &call,
                                         convertTimeout(timeout))) {
        return nullptr;
    }

    dbus_pending_call_set_notify(call, DBusPendingCallNotifyCallback,
                                 slot.get(), nullptr);
    slot->reply_ = call;
    slot->bus_ = bus->watch();

    return slot;
}

Message::operator bool() const {
    FCITX_D();
    return d->msg() && d->lastError_ >= 0;
}

bool Message::end() const {
    FCITX_D();
    return d->end();
}

void Message::resetError() {
    FCITX_D();
    d->lastError_ = 0;
}

void Message::rewind() {
    FCITX_D();
    d->rewind();
}

void Message::skip() {
    FCITX_D();
    dbus_message_iter_next(d->iterator());
}

std::pair<char, std::string> Message::peekType() {
    FCITX_D();

    UniqueCPtr<char, dbus_free> content;
    auto container = dbus_message_iter_get_arg_type(d->iterator());
    if (container == DBUS_TYPE_ARRAY || container == DBUS_TYPE_STRUCT ||
        container == DBUS_TYPE_VARIANT) {
        auto subIter = d->pushReadIterator();
        content.reset(dbus_message_iter_get_signature(subIter));
        d->pop();
    }
    if (content) {
        return {container, content.get()};
    }
    return {container, ""};
}

bool Message::send() {
    FCITX_D();
    auto bus = d->bus_.get();
    if (bus) {
        if (dbus_connection_send(bus->conn_.get(), d->msg(), nullptr)) {
            return true;
        }
    }
    return false;
}

Message &Message::operator<<(bool b) {
    FCITX_D();
    dbus_bool_t i = b ? 1 : 0;
    d->lastError_ =
        dbus_message_iter_append_basic(d->iterator(), DBUS_TYPE_BOOLEAN, &i);
    return *this;
}

Message &Message::operator>>(bool &b) {
    FCITX_D();
    dbus_bool_t i = 0;
    if (dbus_message_iter_get_arg_type(d->iterator()) == DBUS_TYPE_BOOLEAN) {
        dbus_message_iter_get_basic(d->iterator(), &i);
        b = i ? true : false;
        dbus_message_iter_next(d->iterator());
    }
    return *this;
}

#define _MARSHALL_FUNC(TYPE, TYPE2)                                            \
    Message &Message::operator<<(TYPE v) {                                     \
        if (!(*this)) {                                                        \
            return *this;                                                      \
        }                                                                      \
        FCITX_D();                                                             \
        d->lastError_ = !dbus_message_iter_append_basic(                       \
            d->iterator(), DBUS_TYPE_##TYPE2, &v);                             \
        return *this;                                                          \
    }                                                                          \
    Message &Message::operator>>(TYPE &v) {                                    \
        if (!(*this)) {                                                        \
            return *this;                                                      \
        }                                                                      \
        FCITX_D();                                                             \
        if (dbus_message_iter_get_arg_type(d->iterator()) ==                   \
            DBUS_TYPE_##TYPE2) {                                               \
            dbus_message_iter_get_basic(d->iterator(), &v);                    \
            dbus_message_iter_next(d->iterator());                             \
        } else {                                                               \
            d->lastError_ = -EINVAL;                                           \
        }                                                                      \
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
    d->lastError_ =
        !dbus_message_iter_append_basic(d->iterator(), DBUS_TYPE_STRING, &s);
    return *this;
}

Message &Message::operator>>(std::string &s) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    char *p = nullptr;

    if (dbus_message_iter_get_arg_type(d->iterator()) == DBUS_TYPE_STRING) {
        dbus_message_iter_get_basic(d->iterator(), &p);
        s = p;
        dbus_message_iter_next(d->iterator());
    } else {
        d->lastError_ = -EINVAL;
    }
    return *this;
}

Message &Message::operator<<(const ObjectPath &o) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    const char *c = o.path().c_str();
    d->lastError_ = !dbus_message_iter_append_basic(d->iterator(),
                                                    DBUS_TYPE_OBJECT_PATH, &c);
    return *this;
}

Message &Message::operator>>(ObjectPath &o) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    char *p = nullptr;
    if (dbus_message_iter_get_arg_type(d->iterator()) == DBUS_TYPE_STRING) {
        dbus_message_iter_get_basic(d->iterator(), &p);
        o = ObjectPath(p);
        dbus_message_iter_next(d->iterator());
    } else {
        d->lastError_ = -EINVAL;
    }
    return *this;
}

Message &Message::operator<<(const Signature &s) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    const char *c = s.sig().c_str();
    d->lastError_ =
        !dbus_message_iter_append_basic(d->iterator(), DBUS_TYPE_SIGNATURE, &c);
    return *this;
}

Message &Message::operator>>(Signature &s) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    char *p = nullptr;
    if (dbus_message_iter_get_arg_type(d->iterator()) == DBUS_TYPE_SIGNATURE) {
        dbus_message_iter_get_basic(d->iterator(), &p);
        s = Signature(p);
        dbus_message_iter_next(d->iterator());
    } else {
        d->lastError_ = -EINVAL;
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
        dbus_message_iter_append_basic(d->iterator(), DBUS_TYPE_UNIX_FD, &f);
    return *this;
}

Message &Message::operator>>(UnixFD &fd) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    int f = -1;
    if (dbus_message_iter_get_arg_type(d->iterator()) == DBUS_TYPE_UNIX_FD) {
        dbus_message_iter_get_basic(d->iterator(), &f);
        fd.give(f);
        dbus_message_iter_next(d->iterator());
    } else {
        d->lastError_ = -EINVAL;
    }
    return *this;
}

Message &Message::operator<<(const Container &c) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    d->pushWriteIterator(toDBusType(c.type()), c.content().sig());
    return *this;
}

Message &Message::operator>>(const Container &c) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();

    if (dbus_message_iter_get_arg_type(d->iterator()) != toDBusType(c.type())) {
        d->lastError_ = -EINVAL;
    } else {
        DBusMessageIter *subIter = d->pushReadIterator();
        // these two type doesn't support such check.
        if (c.type() != Container::Type::DictEntry &&
            c.type() != Container::Type::Struct) {
            UniqueCPtr<char, dbus_free> content(
                dbus_message_iter_get_signature(subIter));
            if (!content || content.get() != c.content().sig()) {
                d->lastError_ = -EINVAL;
            }
        }
    }
    return *this;
}

Message &Message::operator<<(const ContainerEnd &) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    d->pop();
    return *this;
}

Message &Message::operator>>(const ContainerEnd &) {
    if (!(*this)) {
        return *this;
    }
    FCITX_D();
    d->pop();
    dbus_message_iter_next(d->iterator());
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
    if (dbus_message_iter_get_arg_type(d->iterator()) == DBUS_TYPE_VARIANT) {
        dbus_message_iter_next(d->iterator());
    } else {
        d->lastError_ = -EINVAL;
    }
    return *this;
}
} // namespace dbus
} // namespace fcitx
