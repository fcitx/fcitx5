/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <cstdlib>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/event.h"

using namespace fcitx;

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
           "\t-h\t\tdisplay this help and exit\n";
}

class Launcher {
public:
    Launcher(int fd, std::string display)
        : fd_(UnixFD::own(fd)), display_(display) {
        if (!bus_.isOpen()) {
            throw std::runtime_error("Failed to open dbus connection.");
        }
        std::string connectedName;
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
            auto message = bus_.createMethodCall("org.freedesktop.DBus", "/",
                                                 "org.freedesktop.DBus",
                                                 "StartServiceByName");
            message << "org.fcitx.Fcitx5";
            message << 0u;
            message.send();
            return;
        }

        if (!newOwner.empty() && connectedName_.empty()) {
            delayedConnection_ = loop_.addTimeEvent(
                CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
                [this, newOwner](EventSource *, uint64_t) {
                    connectTo(newOwner);
                    return true;
                });
        }
    }

    void connectTo(const std::string &newOwner) {
        connectedName_ = newOwner;
        if (fd_.isValid()) {
            auto message = bus_.createMethodCall(newOwner.data(), "/controller",
                                                 "org.fcitx.Fcitx.Controller1",
                                                 "OpenWaylandConnectionSocket");
            message << fd_;
            reply_ = message.callAsync(0, [this](dbus::Message &message) {
                reply(message);
                return true;
            });
            fd_.release();
        } else {
            auto message = bus_.createMethodCall(newOwner.data(), "/controller",
                                                 "org.fcitx.Fcitx.Controller1",
                                                 "OpenWaylandConnection");
            message << display_;
            reply_ = message.callAsync(0, [this](dbus::Message &message) {
                reply(message);
                return true;
            });
        }
    }

    void reply(dbus::Message &message) {
        if (message.isError()) {
            done_ = true;
            error_ = true;
            FCITX_ERROR() << "DBus call error: " << message.errorName()
                          << message.errorMessage();
            loop_.exit();
        }
    }

    dbus::Bus bus_{dbus::BusType::Session};
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
    EventLoop loop_;
    std::unique_ptr<dbus::ServiceWatcherEntry> slot_;
    std::unique_ptr<dbus::Slot> reply_;
    std::string connectedName_;
    UnixFD fd_;
    std::string display_;
    bool done_ = false;
    std::unique_ptr<EventSource> delayedConnection_;
    bool error_ = false;
};

int main(int argc, char *argv[]) {
    int c;
    while ((c = getopt(argc, argv, "h")) != -1) {
        switch (c) {
        case 'h':
            usage(std::cout);
            return 0;
        default:
            usage(std::cerr);
            return 1;
        }
    }

    auto socket = getenv("WAYLAND_SOCKET");
    int fd = -1;
    if (socket) {
        fd = std::stoi(socket);
    }
    const char *display = getenv("WAYLAND_DISPLAY");
    if ((!display || !display[0]) && fd < 0) {
        FCITX_ERROR() << "WAYLAND_SOCKET or WAYLAND_DISPLAY is not set.";
        return 1;
    }

    try {
        Launcher launcher(fd, display);
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
