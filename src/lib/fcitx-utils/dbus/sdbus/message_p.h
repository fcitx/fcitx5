/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_MESSAGE_P_H_
#define _FCITX_UTILS_DBUS_MESSAGE_P_H_

#include <cstdint>
#include <string>
#include "../message.h"
#include "sd-bus-wrap.h"

namespace fcitx::dbus {

class MessagePrivate {
public:
    MessagePrivate() : type_(MessageType::Invalid), msg_(nullptr) {}

    ~MessagePrivate() { sd_bus_message_unref(msg_); }

    static Message fromSDBusMessage(sd_bus_message *sdmsg, bool ref = true) {
        Message message;
        message.d_ptr->msg_ = ref ? sd_bus_message_ref(sdmsg) : sdmsg;
        uint8_t type = 0;
        MessageType msgType = MessageType::Invalid;
        sd_bus_message_get_type(sdmsg, &type);
        switch (type) {
        case SD_BUS_MESSAGE_METHOD_CALL:
            msgType = MessageType::MethodCall;
            break;
        case SD_BUS_MESSAGE_METHOD_RETURN:
            msgType = MessageType::Reply;
            break;
        case SD_BUS_MESSAGE_METHOD_ERROR:
            msgType = MessageType::Error;
            break;
        case SD_BUS_MESSAGE_SIGNAL:
            msgType = MessageType::Signal;
            break;
        }

        message.d_ptr->type_ = msgType;

        return message;
    }

    static Message fromSDError(const sd_bus_error &error) {
        Message msg;
        auto *msgD = msg.d_func();
        msgD->type_ = MessageType::Error;
        msgD->error_ = error.name;
        msgD->message_ = error.message;
        return msg;
    }

    MessageType type_;
    sd_bus_message *msg_;
    std::string error_;
    std::string message_;
    int lastError_ = 0;
};
} // namespace fcitx::dbus

#endif // _FCITX_UTILS_DBUS_MESSAGE_P_H_
