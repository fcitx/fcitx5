/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_BUS_P_H_
#define _FCITX_UTILS_DBUS_BUS_P_H_

#include <string>
#include <utility>
#include "../bus.h"
#include "../message.h"
#include "sd-bus-wrap.h"

namespace fcitx::dbus {

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
    SDVTableSlot(Bus *bus_, std::string path, std::string interface)
        : bus_(bus_), path_(std::move(path)), interface_(std::move(interface)) {
    }

    ~SDVTableSlot() {
        if (slot_) {
            sd_bus_slot_set_userdata(slot_, nullptr);
            sd_bus_slot_unref(slot_);
        }
    }

    sd_bus_slot *slot_ = nullptr;
    Bus *bus_;
    std::string path_;
    std::string interface_;
};

class SDSlot : public Slot {
public:
    SDSlot(MessageCallback callback_)
        : callback_(std::move(callback_)), slot_(nullptr) {}

    ~SDSlot() {
        if (slot_) {
            sd_bus_slot_set_userdata(slot_, nullptr);
            sd_bus_slot_unref(slot_);
        }
    }

    MessageCallback callback_;
    sd_bus_slot *slot_;
};
} // namespace fcitx::dbus

#endif // _FCITX_UTILS_DBUS_BUS_P_H_
