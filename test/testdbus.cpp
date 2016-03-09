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
#include "fcitx-utils/dbus.h"
#include "fcitx-utils/event.h"
#include <iostream>

using namespace fcitx::dbus;
using namespace fcitx;

int main()
{
    Bus bus(BusType::Session);
    EventLoop loop;
    bus.attachEventLoop(&loop);
    std::unique_ptr<EventSourceTime> s(loop.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 0, [&bus, &loop] (EventSource *, uint64_t) {
        auto msg = bus.createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus.Introspectable", "Introspect");
        auto reply = bus.call(std::move(msg), 0);
        std::cout << static_cast<int>(reply.type()) << std::endl;
        loop.quit();
        return false;
    }));
    loop.exec();
    return 0;
}
