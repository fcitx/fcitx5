//
// Copyright (C) 2017~2017 by CSSlayer
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

#include "config.h"

#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/utf8.h"
#include <iostream>
#include <sys/signal.h>
#include <unistd.h>

using namespace fcitx;
using namespace fcitx::dbus;

void usage(std::ostream &stream) {
    stream << "Usage: fcitx-remote [OPTION]\n"
              "\t-c\t\tinactivate input method\n"
              "\t-o\t\tactivate input method\n"
              "\t-r\t\treload fcitx config\n"
              "\t-t,-T\t\tswitch Active/Inactive\n"
              "\t-e\t\tAsk fcitx to exit\n"
              "\t-a\t\tprint fcitx's dbus address\n"
              "\t-m <imname>\tprint corresponding addon name for im\n"
              "\t-s <imname>\tswitch to the input method uniquely identified "
              "by <imname>\n"
              "\t[no option]\tdisplay fcitx state, 0 for close, 1 for "
              "inactive, 2 for active\n"
              "\t-h\t\tdisplay this help and exit\n";
}

enum {
    FCITX_DBUS_ACTIVATE,
    FCITX_DBUS_DEACTIVATE,
    FCITX_DBUS_RELOAD_CONFIG,
    FCITX_DBUS_EXIT,
    FCITX_DBUS_TOGGLE,
    FCITX_DBUS_GET_CURRENT_STATE,
    FCITX_DBUS_GET_IM_ADDON,
    FCITX_DBUS_SET_CURRENT_IM
};

int main(int argc, char *argv[]) {
    Bus bus(BusType::Session);
    Message message;
    if (!bus.isOpen()) {
        FCITX_ERROR() << "Could not open DBus connection.";
        return 1;
    }
    int c;
    int ret = 1;
    int messageType = FCITX_DBUS_GET_CURRENT_STATE;
    std::string imname;
    while ((c = getopt(argc, argv, "chortTeam:s:")) != -1) {
        switch (c) {
        case 'o':
            messageType = FCITX_DBUS_ACTIVATE;
            break;

        case 'c':
            messageType = FCITX_DBUS_DEACTIVATE;
            break;

        case 'r':
            messageType = FCITX_DBUS_RELOAD_CONFIG;
            break;

        case 't':
        case 'T':
            messageType = FCITX_DBUS_TOGGLE;
            break;

        case 'e':
            messageType = FCITX_DBUS_EXIT;
            break;

        case 'm':
            messageType = FCITX_DBUS_GET_IM_ADDON;
            imname = optarg;
            break;

        case 's':
            messageType = FCITX_DBUS_SET_CURRENT_IM;
            imname = optarg;
            break;

        case 'a':
            std::cout << bus.address() << std::endl;
            return 0;
        case 'h':
            usage(std::cout);
            return 0;
        default:
            usage(std::cerr);
            return 1;
        }
    }
    if (!imname.empty() && !utf8::validate(imname)) {
        std::cerr << "Input method name is invalid." << std::endl;
        return 1;
    }

    constexpr char serviceName[] = "org.fcitx.Fcitx5";
    constexpr char interfaceName[] = "org.fcitx.Fcitx.Controller1";
    constexpr char path[] = "/controller";
    // 1s
    constexpr uint64_t defaultTimeout = 1000 * 1000;

#define CASE(ENUMNAME, MESSAGENAME)                                            \
    case FCITX_DBUS_##ENUMNAME:                                                \
        message = bus.createMethodCall(serviceName, path, interfaceName,       \
                                       #MESSAGENAME);                          \
        break;

    switch (messageType) {
        CASE(ACTIVATE, Activate);
        CASE(DEACTIVATE, Deactivate);
        CASE(RELOAD_CONFIG, ReloadConfig);
        CASE(EXIT, Exit);
        CASE(TOGGLE, Toggle);
        CASE(GET_CURRENT_STATE, State);
        CASE(GET_IM_ADDON, AddonForIM);
        CASE(SET_CURRENT_IM, SetCurrentIM);

    default:
        goto some_error;
    };
    if (!message) {
        goto some_error;
    }

    if (messageType == FCITX_DBUS_GET_CURRENT_STATE) {
        auto reply = message.call(defaultTimeout);
        if (!reply.isError()) {
            int result = 0;
            reply >> result;
            std::cout << result << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to get reply." << std::endl;
            return 1;
        }
    } else if (messageType == FCITX_DBUS_GET_IM_ADDON) {
        message << imname;
        auto reply = message.call(defaultTimeout);
        if (!reply.isError()) {
            std::string result;
            reply >> result;
            std::cout << result << std::endl;
            return 0;
        } else {
            std::cerr << "Failed to get reply." << std::endl;
            return 1;
        }
    } else if (messageType == FCITX_DBUS_SET_CURRENT_IM) {
        message << imname;
        auto reply = message.call(defaultTimeout);
        return reply.isError() ? 1 : 0;
    } else {
        auto reply = message.call(defaultTimeout);
        return reply.isError() ? 1 : 0;
    }

some_error:
    return ret;
}
