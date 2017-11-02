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

#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/log.h"

using namespace fcitx::dbus;
using namespace fcitx;
#define TEST_SERVICE "org.fcitx.Fcitx.TestDBus"

int main() {
    Bus bus(BusType::Session);
    if (!bus.isOpen()) {
        return 1;
    }
    EventLoop loop;
    bus.attachEventLoop(&loop);

    std::unique_ptr<HandlerTableEntry<ServiceWatcherCallback>>
        handlerTableEntry;
    ServiceWatcher watcher(bus);

    if (!bus.requestName(TEST_SERVICE,
                         {RequestNameFlag::AllowReplacement,
                          RequestNameFlag::ReplaceExisting})) {
        return 1;
    }

    std::string name = bus.serviceOwner(TEST_SERVICE, 0);
    FCITX_ASSERT(name == bus.uniqueName());

    handlerTableEntry = watcher.watchService(
        TEST_SERVICE, [&loop](const std::string &name, const std::string &,
                              const std::string &) {
            FCITX_ASSERT(name == TEST_SERVICE);
            loop.quit();
        });

    FCITX_ASSERT(bus.releaseName(TEST_SERVICE));
    loop.exec();
    return 0;
}
