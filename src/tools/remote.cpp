/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <iostream>
#include <getopt.h>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/utf8.h"

using namespace fcitx;
using namespace fcitx::dbus;

static constexpr char fcitxServiceName[] = "org.fcitx.Fcitx5";
static constexpr char interfaceName[] = "org.fcitx.Fcitx.Controller1";
static constexpr char path[] = "/controller";

// 1s
static constexpr uint64_t defaultTimeout = 1000 * 1000;

void usage(std::ostream &stream) {
    stream
        << "Usage: fcitx5-remote [OPTION]\n"
           "\t-c\t\tinactivate input method\n"
           "\t-o\t\tactivate input method\n"
           "\t-r\t\treload fcitx config\n"
           "\t-t,-T\t\tswitch Active/Inactive\n"
           "\t-e\t\tAsk fcitx to exit\n"
           "\t-a\t\tprint fcitx's dbus address\n"
           "\t-m <imname>\tprint corresponding addon name for im\n"
           "\t-g <group>\tset current input method group\n"
           "\t-q\t\tGet current input method group name\n"
           "\t-n\t\tGet current input method name\n"
           "\t-s <imname>\tswitch to the input method uniquely identified "
           "by <imname>\n"
           "\t-x\tOpen a new X11 connection with current DISPLAY environment.\n"
           "\t--check\tCheck if Fcitx is already running, if not, return 1.\n"
           "\t\t\tCan be used with other options.\n"
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
    FCITX_DBUS_SET_CURRENT_IM,
    FCITX_DBUS_GET_CURRENT_IM,
    FCITX_DBUS_SET_CURRENT_GROUP,
    FCITX_DBUS_GET_CURRENT_GROUP,
    FCITX_DBUS_OPEN_X11_CONNECTION,
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
    std::string serviceName = fcitxServiceName;
    struct option longOptions[] = {{"check", no_argument, nullptr, 0},
                                   {"help", no_argument, nullptr, 'h'},
                                   {nullptr, 0, 0, 0}};

    int optionIndex = 0;
    while ((c = getopt_long(argc, argv, "nchortTeam:s:g:qx", longOptions,
                            &optionIndex)) != EOF) {
        switch (c) {
        case 0:
            switch (optionIndex) {
            case 0: {
                // Make sure we use the unique name to send request to avoid any
                // race, otherwise it may trigger dbus activation.
                serviceName = bus.serviceOwner(serviceName, defaultTimeout);
                if (serviceName.empty()) {
                    return 1;
                }
            } break;
            }
            break;
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

        case 'n':
            messageType = FCITX_DBUS_GET_CURRENT_IM;
            break;

        case 's':
            messageType = FCITX_DBUS_SET_CURRENT_IM;
            imname = optarg;
            break;

        case 'g':
            messageType = FCITX_DBUS_SET_CURRENT_GROUP;
            imname = optarg;
            break;

        case 'q':
            messageType = FCITX_DBUS_GET_CURRENT_GROUP;
            break;

        case 'x':
            messageType = FCITX_DBUS_OPEN_X11_CONNECTION;
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

#define CASE(ENUMNAME, MESSAGENAME)                                            \
    case FCITX_DBUS_##ENUMNAME:                                                \
        message = bus.createMethodCall(serviceName.data(), path,               \
                                       interfaceName, #MESSAGENAME);           \
        break;

    switch (messageType) {
        CASE(ACTIVATE, Activate);
        CASE(DEACTIVATE, Deactivate);
        CASE(RELOAD_CONFIG, ReloadConfig);
        CASE(EXIT, Exit);
        CASE(TOGGLE, Toggle);
        CASE(GET_CURRENT_STATE, State);
        CASE(GET_IM_ADDON, AddonForIM);
        CASE(GET_CURRENT_IM, CurrentInputMethod);
        CASE(SET_CURRENT_IM, SetCurrentIM);
        CASE(GET_CURRENT_GROUP, CurrentInputMethodGroup);
        CASE(SET_CURRENT_GROUP, SwitchInputMethodGroup);
        CASE(OPEN_X11_CONNECTION, OpenX11Connection);

    default:
        return ret;
    };
    if (!message) {
        return ret;
    }

    // We need to have some special code for this, this is because we don't want
    // to trigger dbus activation for it.
    if (messageType == FCITX_DBUS_EXIT) {
        auto owner = bus.serviceOwner(serviceName, defaultTimeout);
        // Either failed or not present, it will be return empty.
        if (owner.empty()) {
            return 0;
        }
    }

    if (messageType == FCITX_DBUS_GET_CURRENT_STATE) {
        auto reply = message.call(defaultTimeout);
        if (!reply.isError()) {
            int result = 0;
            reply >> result;
            std::cout << result << std::endl;
            return 0;
        }
        std::cerr << "Failed to get reply." << std::endl;
        return 1;
    }
    if (messageType == FCITX_DBUS_GET_IM_ADDON) {
        message << imname;
        auto reply = message.call(defaultTimeout);
        if (!reply.isError()) {
            std::string result;
            reply >> result;
            std::cout << result << std::endl;
            return 0;
        }
        std::cerr << "Failed to get reply." << std::endl;
        return 1;
    }
    if (messageType == FCITX_DBUS_GET_CURRENT_IM) {
        auto reply = message.call(defaultTimeout);
        if (!reply.isError()) {
            std::string result;
            reply >> result;
            std::cout << result << std::endl;
            return 0;
        }
        std::cerr << "Failed to get reply." << std::endl;
        return 1;
    }
    if (messageType == FCITX_DBUS_SET_CURRENT_IM) {
        message << imname;
        auto reply = message.call(defaultTimeout);
        return reply.isError() ? 1 : 0;
    }
    if (messageType == FCITX_DBUS_SET_CURRENT_GROUP) {
        message << imname;
        auto reply = message.call(defaultTimeout);
        return reply.isError() ? 1 : 0;
    }
    if (messageType == FCITX_DBUS_GET_CURRENT_GROUP) {
        auto reply = message.call(defaultTimeout);
        if (!reply.isError()) {
            std::string result;
            reply >> result;
            std::cout << result << std::endl;
            return 0;
        }
        std::cerr << "Failed to get reply." << std::endl;
        return 1;
    }
    if (messageType == FCITX_DBUS_OPEN_X11_CONNECTION) {
        auto x11Display = getenv("DISPLAY");
        if (!x11Display) {
            std::cerr << "DISPLAY is not set." << std::endl;
            return 1;
        }
        message << x11Display;
        auto reply = message.call(defaultTimeout);
        if (reply.isError()) {
            std::cerr << "Failed to open X11 display: " << x11Display
                      << std::endl
                      << reply.errorName() << " " << reply.errorMessage()
                      << std::endl;
        }
        return reply.isError() ? 1 : 0;
    }

    auto reply = message.call(defaultTimeout);
    return reply.isError() ? 1 : 0;
}
