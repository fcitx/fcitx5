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
 * License along with this library; see the  file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#include "fcitx-utils/dbus.h"
#include "fcitx-utils/event.h"
#include <iostream>
#include <cassert>

using namespace fcitx::dbus;
using namespace fcitx;

class TestObject : public ObjectVTable
{
    void test1() { }
    std::string test2(int i) { return std::to_string(i); }
private:
    ObjectVTableMethod test1Method{this, "test1", "", "", [this] (Message msg) {
        test1();
        msg.createReply().send();
        return true;
    }};
    ObjectVTableMethod test2Method{this, "test2", "i", "s", [this] (Message msg) {
        int i;
        msg >> i;
        auto reply = msg.createReply();
        reply << test2(i);
        reply.send();
        return true;
    }};
};

#define TEST_SERVICE "org.fcitx.Fcitx.TestDBus"
#define TEST_INTERFACE "org.fcitx.Fcitx.TestDBus.Interface"

void *client(void *)
{
    Bus clientBus(BusType::Session);
    EventLoop loop;
    clientBus.attachEventLoop(&loop);
    std::unique_ptr<EventSourceTime> s2(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
        [&clientBus, &loop](EventSource *, uint64_t) {
            auto msg = clientBus.createMethodCall(
                TEST_SERVICE, "/test",
                TEST_INTERFACE, "test2");
            msg << 2;
            auto reply = msg.call(0);
            assert(reply.type() == MessageType::Reply);
            assert(reply.signature() == "s");
            std::string ret;
            reply >> ret;
            assert(ret == "2");
            loop.quit();
            return false;
        }));
    loop.exec();
    return nullptr;
}

int main() {
    Bus bus(BusType::Session);
    EventLoop loop;
    bus.attachEventLoop(&loop);
    if (!bus.requestName(TEST_SERVICE, {RequestNameFlag::AllowReplacement, RequestNameFlag::ReplaceExisting})) {
        return 1;
    }
    TestObject obj;
    assert(bus.addObjectVTable("/test", TEST_INTERFACE, obj));
    std::unique_ptr<EventSourceTime> s(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 2000000, 0,
        [&bus, &loop](EventSource *, uint64_t) {
            auto msg = bus.createMethodCall(
                "org.freedesktop.DBus", "/org/freedesktop/DBus",
                "org.freedesktop.DBus.Introspectable", "Introspect");
            auto reply = msg.call(0);
            loop.quit();
            return false;
        }));

    pthread_t c;
    pthread_create(&c, nullptr, client, nullptr);

    loop.exec();

    pthread_join(c, nullptr);
    return 0;
}
