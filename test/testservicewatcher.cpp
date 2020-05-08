/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
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

    if (!bus.requestName(TEST_SERVICE, {RequestNameFlag::AllowReplacement,
                                        RequestNameFlag::ReplaceExisting})) {
        return 1;
    }

    std::string name = bus.serviceOwner(TEST_SERVICE, 0);
    FCITX_ASSERT(name == bus.uniqueName());

    handlerTableEntry = watcher.watchService(
        TEST_SERVICE, [&loop](const std::string &name, const std::string &,
                              const std::string &) {
            FCITX_ASSERT(name == TEST_SERVICE);
            loop.exit();
        });

    FCITX_ASSERT(bus.releaseName(TEST_SERVICE));
    loop.exec();
    return 0;
}
