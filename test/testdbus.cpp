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
    std::string test2(int32_t i) { return std::to_string(i); }
    std::tuple<int32_t, uint32_t> test3(int32_t i) {
        std::cout << "RET" << i << std::endl;
        return std::make_tuple(i - 1, i + 1);
    }
private:
    FCITX_OBJECT_VTABLE_METHOD(test1, "", "");
    FCITX_OBJECT_VTABLE_METHOD(test2, "i", "s");
    FCITX_OBJECT_VTABLE_METHOD(test3, "i", "iu");
};

#define TEST_SERVICE "org.fcitx.Fcitx.TestDBus"
#define TEST_INTERFACE "org.fcitx.Fcitx.TestDBus.Interface"

void *client(void *)
{
    Bus clientBus(BusType::Session);
    EventLoop loop;
    clientBus.attachEventLoop(&loop);
    std::unique_ptr<EventSourceTime> s2(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 100000, 0,
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
            return false;
        }));
    std::unique_ptr<EventSourceTime> s3(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 200000, 0,
        [&clientBus, &loop](EventSource *, uint64_t) {
            auto msg = clientBus.createMethodCall(
                TEST_SERVICE, "/test",
                TEST_INTERFACE, "test3");
            msg << 2;
            auto reply = msg.call(0);
            std::cout << static_cast<int>(reply.type()) << std::endl;
            if (reply.type() == MessageType::Error) {
                std::cout << reply.signature() << std::endl;
            }
            assert(reply.type() == MessageType::Reply);
            assert(reply.signature() == "iu");
            STRING_TO_DBUS_TUPLE("iu") ret;
            reply >> ret;
            std::cout << std::get<0>(ret) << std::endl;
            std::cout << std::get<1>(ret) << std::endl;
            assert(std::get<0>(ret) == 1);
            assert(std::get<1>(ret) == 3);
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
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
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
