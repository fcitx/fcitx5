/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include <cassert>
#include <systemd/sd-bus.h>

using namespace fcitx::dbus;
using namespace fcitx;
#define TEST_SERVICE "org.fcitx.Fcitx.TestDBus"
int quit = false;
int handler(sd_bus_track *track, void *userdata) {
    quit = true;
    sd_bus_track_unref(track);
    return 0;
}

int main() {
    sd_bus *bus;
    sd_bus_track *track;
    sd_bus_open_user(&bus);
    sd_bus_request_name(bus, TEST_SERVICE, 0);
    sd_bus_track_new(bus, &track, handler, NULL);
    sd_bus_track_add_name(track, TEST_SERVICE);
    sd_bus_release_name(bus, TEST_SERVICE);
    while (!quit) {
        sd_bus_process(bus, NULL);
        sd_bus_wait(bus, 0);
    }

    sd_bus_flush_close_unref(bus);
    /*
    Bus bus(BusType::Session);
    if (!bus.isOpen()) {
        return 1;
    }
    EventLoop loop;
    bus.attachEventLoop(&loop);

    std::unique_ptr<HandlerTableEntry<ServiceWatcherCallback>> handlerTableEntry;
    ServiceWatcher watcher(bus);
    handlerTableEntry.reset(watcher.watchService(TEST_SERVICE, [&loop] (const std::string &name, const std::string &old,
    const std::string &newOwner) {
        assert(name == TEST_SERVICE);
        assert(old == "");
        assert(!newOwner.empty());
        loop.quit();
    }));

    if (!bus.requestName(TEST_SERVICE, {RequestNameFlag::AllowReplacement, RequestNameFlag::ReplaceExisting})) {
        return 1;
    }

    loop.exec();*/
    return 0;
}
