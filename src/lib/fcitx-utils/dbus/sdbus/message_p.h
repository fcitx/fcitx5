/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_UTILS_DBUS_MESSAGE_P_H_
#define _FCITX_UTILS_DBUS_MESSAGE_P_H_

#include "../message.h"
#include "sd-bus-wrap.h"

namespace fcitx {
namespace dbus {

class MessagePrivate {
public:
    MessagePrivate() : type_(MessageType::Invalid), msg_(nullptr) {}
    MessagePrivate(const MessagePrivate &other)
        : type_(other.type_), msg_(sd_bus_message_ref(other.msg_)),
          lastError_(other.lastError_) {}

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
        auto msgD = msg.d_func();
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
}
}

#endif // _FCITX_UTILS_DBUS_MESSAGE_P_H_
