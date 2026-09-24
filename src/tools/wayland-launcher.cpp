/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <getopt.h>
#include "fcitx-utils/awaiter.h"
#include "fcitx-utils/coroutine.h"
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/coroutine.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/environ.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/unixfd.h"

using namespace fcitx;

namespace {

void usage(std::ostream &stream) {
    stream
        << "Usage: fcitx5-wayland-launcher\n"
           "This is a tool intended to be passed to wayland compositor.\n"
           "This launcher will read the environment variable WAYLAND_SOCKET "
           "and\n"
           "WAYLAND_DISPLAY and ask Fcitx to make a new wayland connection.\n"
           "Normally, a user is not expected to execute it directly.\n"
           "This process will persist and do nothing until Fcitx quits, this "
           "is an\n"
           "expected behavior because compositor may expect the process to "
           "keep running.\n"
           "\t-h\t\tdisplay this help and exit\n"
           "\t--reopen\t\treplace the default wayland connection with "
           "WAYLAND_SOCKET one if WAYLAND_SOCKET is set.\n";
}

class Launcher {
public:
    Launcher(int fd, std::string display, bool reopen)
        : fd_(UnixFD::own(fd)), display_(std::move(display)), reopen_(reopen) {
        if (!bus_.isOpen()) {
            throw std::runtime_error("Failed to open dbus connection.");
        }
        bus_.attachEventLoop(&loop_);

        watcher_ = std::make_unique<dbus::ServiceWatcher>(bus_);

        slot_ = watcher_->watchService("org.fcitx.Fcitx5",
                                       [this](const std::string &,
                                              const std::string &oldOwner,
                                              const std::string &newOwner) {
                                           ownerChanged(oldOwner, newOwner);
                                       });
    }

    bool error() const { return error_; }

    void run() {
        if (!done_) {
            loop_.exec();
        }
    }

private:
    void ownerChanged(const std::string &oldOwner,
                      const std::string &newOwner) {
        if (!oldOwner.empty() && oldOwner == connectedName_) {
            done_ = true;
            loop_.exit();
        }

        if (oldOwner.empty() && newOwner.empty()) {
            // This is initial query, let's just start service.
            startServiceTask_ =
                std::make_unique<CoroutineTask<void>>(startService());
            startServiceTask_->resume();
            return;
        }

        if (!newOwner.empty() && connectedName_.empty()) {
            connectedName_ = newOwner;
            connectionTask_ =
                std::make_unique<CoroutineTask<void>>(connectTo());
            connectionTask_->resume();
        }
    }

    Coroutine<void> startService() {
        try {
            auto message = bus_.createMethodCall("org.freedesktop.DBus", "/",
                                                 "org.freedesktop.DBus",
                                                 "StartServiceByName");
            message << "org.fcitx.Fcitx5";
            message << 0U;
            auto result =
                co_await dbus::AsyncReturn<uint32_t>(std::move(message));
            if (result != 1 && result != 2) {
                throw std::runtime_error(
                    "Unexpected response from StartServiceByName: " +
                    std::to_string(result));
            }
        } catch (const std::exception &e) {
            done_ = true;
            error_ = true;
            FCITX_ERROR() << "Failed to start Fcitx service: " << e.what();
            loop_.exit();
        }
        assert(startServiceTask_);
        std::move(*startServiceTask_).detach_handle();
    }

    Coroutine<void> connectTo() {
        try {
            co_await TimeAwaiter::after(loop_, 1000000);
            if (fd_.isValid()) {
                if (reopen_) {
                    auto message = bus_.createMethodCall(
                        connectedName_.data(), "/controller",
                        "org.fcitx.Fcitx.Controller1",
                        "ReopenWaylandConnectionSocket");
                    message << display_;
                    message << fd_;
                    fd_.release();
                    co_await dbus::AsyncReturn<>(std::move(message));
                } else {
                    auto message = bus_.createMethodCall(
                        connectedName_.data(), "/controller",
                        "org.fcitx.Fcitx.Controller1",
                        "OpenWaylandConnectionSocket");
                    message << fd_;
                    fd_.release();
                    co_await dbus::AsyncReturn<>(std::move(message));
                }
            } else {
                auto message = bus_.createMethodCall(
                    connectedName_.data(), "/controller",
                    "org.fcitx.Fcitx.Controller1", "OpenWaylandConnection");
                message << display_;
                co_await dbus::AsyncReturn<>(std::move(message));
            }
        } catch (const std::exception &e) {
            done_ = true;
            error_ = true;
            FCITX_ERROR() << "Failed to open Wayland connection: " << e.what();
            loop_.exit();
        }
        assert(connectionTask_);
        std::move(*connectionTask_).detach_handle();
    }

    dbus::Bus bus_{dbus::BusType::Session};
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
    EventLoop loop_;
    std::unique_ptr<dbus::ServiceWatcherEntry> slot_;
    std::unique_ptr<CoroutineTask<void>> startServiceTask_;
    std::unique_ptr<CoroutineTask<void>> connectionTask_;
    std::string connectedName_;
    UnixFD fd_;
    std::string display_;
    bool done_ = false;
    bool error_ = false;
    bool reopen_ = false;
};

} // namespace

int main(int argc, char *argv[]) {
    bool reopen = false;
    struct option longOptions[] = {{"reopen", no_argument, nullptr, 0},
                                   {"help", no_argument, nullptr, 'h'},
                                   {nullptr, 0, 0, 0}};

    int optionIndex = 0;
    int c;
    while ((c = getopt_long(argc, argv, "h", longOptions, &optionIndex)) !=
           EOF) {
        switch (c) {
        case 0: {
            switch (optionIndex) {
            case 0:
                reopen = true;
                break;
            default:
                usage(std::cerr);
                return 1;
            }
        } break;
        case 'r':
            break;
        case 'h':
            usage(std::cout);
            return 0;
        default:
            usage(std::cerr);
            return 1;
        }
    }

    int fd = -1;
    if (auto socket = getEnvironment("WAYLAND_SOCKET")) {
        std::from_chars(socket->data(), socket->data() + socket->size(), fd);
    }
    auto display = getEnvironmentOrEmpty("WAYLAND_DISPLAY");
    if (display.empty() && fd < 0) {
        FCITX_ERROR() << "WAYLAND_SOCKET or WAYLAND_DISPLAY is not set.";
        return 1;
    }

    try {
        Launcher launcher(fd, display, reopen);
        launcher.run();
        if (launcher.error()) {
            return 1;
        }
    } catch (const std::exception &e) {
        FCITX_ERROR() << e.what();
        return 1;
    }

    return 0;
}
