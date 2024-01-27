/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <thread>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/log.h"

using namespace fcitx::dbus;
using namespace fcitx;

class TestObject : public ObjectVTable<TestObject> {
    void test1() {}
    std::string test2(int32_t i) {
        FCITX_INFO() << "test2 called";
        return std::to_string(i);
    }
    std::tuple<int32_t, uint32_t> test3(int32_t i) {
        std::vector<DBusStruct<std::string, int>> data;
        data.emplace_back(std::make_tuple(std::to_string(i), i));
        testSignal(data);
        FCITX_INFO() << "test3 called";
        return std::make_tuple(i - 1, i + 1);
    }
    bool testError() {
        FCITX_INFO() << "testError called";
        throw MethodCallError("org.freedesktop.DBus.Error.FileNotFound",
                              "File not found");
    }
    Variant test4(const Variant &v) {
        Variant result;
        auto *msg = currentMessage();
        FCITX_INFO() << "test4 called";
        FCITX_INFO() << v;
        msg->rewind();
        auto type = msg->peekType();
        if (type.first == 'v' && type.second == "i") {
            *msg >> Container(Container::Type::Variant, Signature(type.second));
            int32_t i;
            *msg >> i;
            *msg >> ContainerEnd();
            return Variant(std::to_string(i));
        }
        return Variant();
    }
    std::string
    test5(const std::vector<DictEntry<std::string, std::string>> &entries) {
        for (const auto &entry : entries) {
            if (entry.key() == "a") {
                return entry.value();
            }
        }
        return "";
    }

private:
    int prop2 = 1;
    FCITX_OBJECT_VTABLE_METHOD(test1, "test1", "", "");
    FCITX_OBJECT_VTABLE_METHOD(test2, "test2", "i", "s");
    FCITX_OBJECT_VTABLE_METHOD(test3, "test3", "i", "iu");
    FCITX_OBJECT_VTABLE_METHOD(test4, "test4", "v", "v");
    FCITX_OBJECT_VTABLE_METHOD(test5, "test5", "a{ss}", "s");
    FCITX_OBJECT_VTABLE_METHOD(testError, "testError", "", "b");
    FCITX_OBJECT_VTABLE_SIGNAL(testSignal, "testSignal", "a(si)");
    FCITX_OBJECT_VTABLE_PROPERTY(testProperty, "testProperty", "i",
                                 []() { return 5; });
    FCITX_OBJECT_VTABLE_WRITABLE_PROPERTY(
        testProperty2, "testProperty2", "i", [this]() { return prop2; },
        [this](int32_t v) { prop2 = v; });
};

#define TEST_SERVICE "org.fcitx.Fcitx.TestDBus"
#define TEST_INTERFACE "org.fcitx.Fcitx.TestDBus.Interface"

void client() {
    Bus clientBus(BusType::Session);
    EventLoop loop;
    clientBus.attachEventLoop(&loop);
    std::unique_ptr<Slot> slot(clientBus.addMatch(
        MatchRule(TEST_SERVICE, "", TEST_INTERFACE, "testSignal"),
        [&loop](dbus::Message &message) {
            FCITX_INFO() << "testSignal";
            std::vector<DBusStruct<std::string, int>> data;
            message >> data;
            FCITX_ASSERT(data.size() == 1);
            FCITX_ASSERT(std::get<0>(data[0]) == "2");
            FCITX_ASSERT(std::get<1>(data[0]) == 2);
            FCITX_ASSERT(std::get<std::string>(data[0]) == "2");
            FCITX_ASSERT(std::get<int>(data[0]) == 2);
            loop.exit();
            return false;
        }));
    FCITX_ASSERT(slot);
    std::unique_ptr<EventSourceTime> s(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 0,
        [&clientBus](EventSource *, uint64_t) {
            FCITX_INFO() << "Client sends Introspect";
            auto msg = clientBus.createMethodCall(
                TEST_SERVICE, "/test", "org.freedesktop.DBus.Introspectable",
                "Introspect");
            auto reply = msg.call(0);
            std::string s;
            reply >> s;
            FCITX_INFO() << "Introspect reply: " << s;
            return false;
        }));
    std::unique_ptr<EventSourceTime> s2(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 100000, 0,
        [&clientBus](EventSource *, uint64_t) {
            FCITX_INFO() << "Client sends test2";
            auto msg = clientBus.createMethodCall(TEST_SERVICE, "/test",
                                                  TEST_INTERFACE, "test2");
            msg << 2;
            auto reply = msg.call(0);
            FCITX_ASSERT(reply.type() == MessageType::Reply);
            FCITX_ASSERT(reply.signature() == "s");
            std::string ret;
            reply >> ret;
            FCITX_INFO() << "test2 reply: " << ret;
            FCITX_ASSERT(ret == "2");
            return false;
        }));
    std::unique_ptr<EventSourceTime> s3(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 200000, 0,
        [&clientBus](EventSource *, uint64_t) {
            FCITX_INFO() << "Client sends testError";
            auto msg = clientBus.createMethodCall(TEST_SERVICE, "/test",
                                                  TEST_INTERFACE, "testError");
            auto reply = msg.call(10000000);
            FCITX_ASSERT(reply.type() == MessageType::Error);
            FCITX_ASSERT(reply.errorName() ==
                         "org.freedesktop.DBus.Error.FileNotFound")
                << reply.errorMessage();
            FCITX_ASSERT(reply.errorMessage() == "File not found")
                << reply.errorMessage();
            return false;
        }));
    std::unique_ptr<EventSourceTime> s4(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 300000, 0,
        [&clientBus](EventSource *, uint64_t) {
            FCITX_INFO() << "Client sends test4";
            auto msg = clientBus.createMethodCall(TEST_SERVICE, "/test",
                                                  TEST_INTERFACE, "test4");
            msg << Variant(123);
            auto reply = msg.call(0);
            FCITX_ASSERT(reply.type() == MessageType::Reply);
            reply >> Container(Container::Type::Variant, Signature("s"));
            std::string s;
            reply >> s;
            FCITX_INFO() << s;
            FCITX_ASSERT(s == "123");
            reply >> ContainerEnd();
            return false;
        }));
    std::unique_ptr<EventSourceTime> s5(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 400000, 0,
        [&clientBus](EventSource *, uint64_t) {
            auto msg = clientBus.createMethodCall(TEST_SERVICE, "/test",
                                                  TEST_INTERFACE, "test5");
            FCITX_INFO() << "Client sends test5";
            std::vector<DictEntry<std::string, std::string>> v;
            v.emplace_back("abc", "def");
            v.emplace_back("a", "defg");

            msg << v;
            FCITX_INFO() << msg.signature();
            auto reply = msg.call(0);
            FCITX_ASSERT(reply.type() == MessageType::Reply);
            std::string s;
            reply >> s;
            FCITX_INFO() << s;
            FCITX_ASSERT(s == "defg");
            return false;
        }));
    std::unique_ptr<EventSourceTime> s6(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 500000, 0,
        [&clientBus](EventSource *, uint64_t) {
            FCITX_INFO() << "test3";
            auto msg = clientBus.createMethodCall(TEST_SERVICE, "/test",
                                                  TEST_INTERFACE, "test3");
            msg << 2;
            auto reply = msg.call(0);
            FCITX_ASSERT(reply.type() == MessageType::Reply);
            FCITX_ASSERT(reply.signature() == "iu");
            FCITX_STRING_TO_DBUS_TUPLE("iu") ret;
            reply >> ret;
            FCITX_ASSERT(std::get<0>(ret) == 1);
            FCITX_ASSERT(std::get<1>(ret) == 3);
            FCITX_INFO() << "test3 ret";
            return false;
        }));
    std::unique_ptr<EventSourceTime> s7(
        loop.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 600000, 0,
                          [&clientBus](EventSource *, uint64_t) {
                              FCITX_INFO() << "testProperty";
                              auto msg = clientBus.createMethodCall(
                                  TEST_SERVICE, "/test",
                                  "org.freedesktop.DBus.Properties", "Get");
                              msg << TEST_INTERFACE << "testProperty";
                              auto reply = msg.call(0);
                              FCITX_ASSERT(reply.type() == MessageType::Reply);
                              FCITX_ASSERT(reply.signature() == "v");
                              dbus::Variant ret;
                              reply >> ret;
                              FCITX_ASSERT(ret.signature() == "i");
                              FCITX_ASSERT(ret.dataAs<int32_t>() == 5);
                              return false;
                          }));
    loop.exec();
}

int main() {
    Bus bus(BusType::Session);
    if (!bus.isOpen()) {
        return 1;
    }
    EventLoop loop;
    bus.attachEventLoop(&loop);
    FCITX_ASSERT(&loop == bus.eventLoop());
    if (!bus.requestName(TEST_SERVICE, {RequestNameFlag::AllowReplacement,
                                        RequestNameFlag::ReplaceExisting})) {
        return 1;
    }
    TestObject obj;
    FCITX_ASSERT(bus.addObjectVTable("/test", TEST_INTERFACE, obj));
    std::unique_ptr<EventSourceTime> s(loop.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 2000000, 0,
        [&bus, &loop](EventSource *, uint64_t) {
            auto msg = bus.createMethodCall(
                "org.freedesktop.DBus", "/org/freedesktop/DBus",
                "org.freedesktop.DBus.Introspectable", "Introspect");
            auto reply = msg.call(0);
            std::string s;
            reply >> s;
            loop.exit();
            return false;
        }));

    {
        MatchRule rule(TEST_SERVICE, "", TEST_INTERFACE, "testSignal");
        FCITX_ASSERT(
            rule.rule() ==
            "type='signal',sender='org.fcitx.Fcitx.TestDBus',interface='org."
            "fcitx.Fcitx.TestDBus.Interface',member='testSignal'")
            << rule.rule();
    }
    {
        MatchRule rule(MessageType::MethodCall, TEST_SERVICE, "",
                       TEST_INTERFACE, "testSignal", "", {"abc"}, true);
        FCITX_ASSERT(rule.rule() ==
                     "type='method_call',sender='org.fcitx.Fcitx.TestDBus',"
                     "path='org.fcitx.Fcitx.TestDBus.Interface',interface='"
                     "testSignal',arg0='abc',eavesdrop='true'")
            << rule.rule();
    }
    {
        MatchRule rule(MessageType::Reply, TEST_SERVICE, "", TEST_INTERFACE,
                       "testSignal", "Test", {"abc"}, true);
        FCITX_ASSERT(rule.rule() ==
                     "type='method_return',sender='org.fcitx.Fcitx.TestDBus',"
                     "path='org.fcitx.Fcitx.TestDBus.Interface',interface='"
                     "testSignal',member='Test',arg0='abc',eavesdrop='true'")
            << rule.rule();
    }

    std::thread thread(client);

    loop.exec();

    thread.join();
    return 0;
}
