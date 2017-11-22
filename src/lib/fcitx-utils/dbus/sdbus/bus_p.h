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
#ifndef _FCITX_UTILS_DBUS_BUS_P_H_
#define _FCITX_UTILS_DBUS_BUS_P_H_

#include "../bus.h"
#include "sd-bus-wrap.h"

namespace fcitx {

namespace dbus {

int SDMessageCallback(sd_bus_message *m, void *userdata, sd_bus_error *);

class ScopedSDBusError {
public:
    ScopedSDBusError() { error_ = SD_BUS_ERROR_NULL; }
    ~ScopedSDBusError() { sd_bus_error_free(&error_); }

    sd_bus_error &error() { return error_; }

private:
    sd_bus_error error_;
};

class SDVTableSlot : public Slot {
public:
    SDVTableSlot(Bus *bus_, const std::string &path_,
                 const std::string &interface_)
        : slot_(nullptr), bus_(bus_), path_(path_), interface_(interface_) {}

    ~SDVTableSlot() {
        if (slot_) {
            sd_bus_slot_set_userdata(slot_, nullptr);
            sd_bus_slot_unref(slot_);
        }
    }

    sd_bus_slot *slot_;
    Bus *bus_;
    std::string path_;
    std::string interface_;
};

class SDSlot : public Slot {
public:
    SDSlot(MessageCallback callback_) : callback_(callback_), slot_(nullptr) {}

    ~SDSlot() {
        if (slot_) {
            sd_bus_slot_set_userdata(slot_, nullptr);
            sd_bus_slot_unref(slot_);
        }
    }

    MessageCallback callback_;
    sd_bus_slot *slot_;
};
}
}

#endif // _FCITX_UTILS_DBUS_BUS_P_H_
