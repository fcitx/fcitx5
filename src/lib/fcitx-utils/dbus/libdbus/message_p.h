/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_MESSAGE_P_H_
#define _FCITX_UTILS_DBUS_MESSAGE_P_H_

#include <dbus/dbus.h>
#include "../message.h"

namespace fcitx {
namespace dbus {

class MessagePrivate {
public:
    MessagePrivate() : type_(MessageType::Invalid), msg_(nullptr) {}
    ~MessagePrivate() {
        if (msg_) {
            dbus_message_unref(msg_);
        }
    }

    static Message fromDBusMessage(TrackableObjectReference<BusPrivate> bus,
                                   DBusMessage *dmsg, bool write, bool ref) {
        Message message;
        message.d_ptr->bus_ = bus;
        message.d_ptr->msg_ = ref ? dbus_message_ref(dmsg) : dmsg;
        message.d_ptr->write_ = write;
        message.d_ptr->initIterator();
        uint8_t type = 0;
        MessageType msgType = MessageType::Invalid;
        type = dbus_message_get_type(dmsg);
        switch (type) {
        case DBUS_MESSAGE_TYPE_METHOD_CALL:
            msgType = MessageType::MethodCall;
            break;
        case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            msgType = MessageType::Reply;
            break;
        case DBUS_MESSAGE_TYPE_ERROR:
            msgType = MessageType::Error;
            break;
        case DBUS_MESSAGE_TYPE_SIGNAL:
            msgType = MessageType::Signal;
            break;
        }

        message.d_ptr->type_ = msgType;

        return message;
    }

    static Message fromDBusError(const DBusError &error) {
        Message msg;
        auto msgD = msg.d_func();
        msgD->type_ = MessageType::Error;
        msgD->error_ = error.name;
        msgD->message_ = error.message;
        return msg;
    }

    void rewind() {
        iterators_.clear();
        iterators_.emplace_back();
        dbus_message_iter_init(msg_, iterator());
    }

    bool end() const {
        return !msg_ || (dbus_message_iter_get_arg_type(iterator()) ==
                         DBUS_TYPE_INVALID);
    }

    void initIterator() {
        iterators_.emplace_back();
        if (write_) {
            dbus_message_iter_init_append(msg_, iterator());
        } else {
            dbus_message_iter_init(msg_, iterator());
        }
    }

    DBusMessageIter *iterator() const { return &iterators_.back(); }

    DBusMessageIter *pushReadIterator() {
        DBusMessageIter *iter = iterator();
        iterators_.emplace_back();
        auto subIter = iterator();
        dbus_message_iter_recurse(iter, subIter);
        return subIter;
    }

    DBusMessageIter *pushWriteIterator(int type, const std::string &subType) {
        DBusMessageIter *iter = iterator();
        iterators_.emplace_back();
        auto subIter = iterator();
        dbus_message_iter_open_container(
            iter, type,
            ((type == DBUS_TYPE_STRUCT || type == DBUS_TYPE_DICT_ENTRY)
                 ? nullptr
                 : subType.c_str()),
            subIter);
        return subIter;
    }

    void pop() {
        assert(iterators_.size() >= 2);
        if (write_) {
            dbus_message_iter_close_container(&*std::next(iterators_.rbegin()),
                                              iterator());
        }
        iterators_.pop_back();
    }

    DBusMessage *msg() const { return msg_; }

    MessageType type_;
    TrackableObjectReference<BusPrivate> bus_;
    bool write_ = false;
    mutable std::list<DBusMessageIter> iterators_;
    std::string error_;
    std::string message_;
    int lastError_ = 0;

private:
    DBusMessage *msg_ = nullptr;
};
} // namespace dbus
} // namespace fcitx

#endif // _FCITX_UTILS_DBUS_MESSAGE_P_H_
