//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include "fcitx-utils/connectableobject.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/metastring.h"
#include "fcitx-utils/signals.h"

void test_simple_signal() {
    bool called = false;
    fcitx::Signal<void()> signal1;
    auto connection = signal1.connect([&called]() {
        called = true;
        return;
    });
    FCITX_ASSERT(connection.connected());
    FCITX_ASSERT(!called);
    signal1();
    called = false;
    auto connection2 = connection;
    FCITX_ASSERT(connection.connected());
    FCITX_ASSERT(connection2.connected());

    connection.disconnect();
    FCITX_ASSERT(!connection.connected());
    FCITX_ASSERT(!connection2.connected());
    signal1();
    FCITX_ASSERT(!called);
}

void test_combiner() {
    fcitx::Signal<int(int)> signal1;
    auto connection = signal1.connect([](int i) { return i; });
    FCITX_ASSERT(signal1(1) == 1);
    auto connection2 = signal1.connect([](int) { return -1; });
    FCITX_ASSERT(signal1(2) == -1);
}

class MaxInteger {
public:
    MaxInteger() : initial_(INT32_MIN) {}

    template <typename InputIterator>
    int operator()(InputIterator begin, InputIterator end) {
        int v = initial_;
        for (; begin != end; begin++) {
            int newv = *begin;
            if (newv > v) {
                v = newv;
            }
        }
        return v;
    }

private:
    int initial_;
};

void test_custom_combiner() {
    fcitx::Signal<int(int), MaxInteger> signal1;
    auto connection = signal1.connect([](int i) { return i; });
    FCITX_ASSERT(signal1(1) == 1);
    auto connection2 = signal1.connect([](int) { return -1; });
    FCITX_ASSERT(signal1(2) == 2);
    connection.disconnect();
    FCITX_ASSERT(signal1(2) == -1);
}

void test_destruct_order() {
    fcitx::Connection connection;
    FCITX_ASSERT(!connection.connected());
    {
        fcitx::Signal<void()> signal1;
        connection = signal1.connect([]() {});

        FCITX_ASSERT(connection.connected());
    }
    FCITX_ASSERT(!connection.connected());
    connection.disconnect();
}

class TestObject : public fcitx::ConnectableObject {
public:
    using fcitx::ConnectableObject::destroy;
};

void test_connectable_object() {
    using namespace fcitx;
    TestObject obj;
    bool called = false;
    auto connection =
        obj.connect<ConnectableObject::Destroyed>([&called, &obj](void *self) {
            FCITX_ASSERT(&obj == self);
            called = true;
        });
    FCITX_ASSERT(connection.connected());
    obj.destroy();
    FCITX_ASSERT(called);
    FCITX_ASSERT(!connection.connected());
    FCITX_ASSERT(
        !obj.connect<ConnectableObject::Destroyed>([](void *) {}).connected());
}

class TestObject2 : public fcitx::ConnectableObject {
public:
    FCITX_DECLARE_SIGNAL(TestObject2, test, void(int &));
    void emitTest(int &i) { emit<TestObject2::test>(i); }
    FCITX_DECLARE_SIGNAL(TestObject2, test2, void(std::string &));
    void emitTest2(std::string &s) { emit<TestObject2::test2>(s); }

private:
    FCITX_DEFINE_SIGNAL(TestObject2, test);
    FCITX_DEFINE_SIGNAL(TestObject2, test2);
};

void test_reference() {
    using namespace fcitx;
    TestObject2 obj;
    int i = 0;
    obj.connect<TestObject2::test>([](int &i) { i++; });
    obj.connect<TestObject2::test>([](int &i) { i++; });
    obj.emitTest(i);
    std::string s;
    obj.connect<TestObject2::test2>([](std::string &s) { s += "a"; });
    obj.connect<TestObject2::test2>([](std::string &s) { s += "b"; });
    obj.emitTest2(s);
    FCITX_ASSERT(s == "ab");
}

void test_move() {
    bool called = false;
    fcitx::Signal<void()> signal1;
    auto connection = signal1.connect([&called]() {
        called = true;
        return;
    });
    FCITX_ASSERT(connection.connected());
    FCITX_ASSERT(!called);
    signal1();
    FCITX_ASSERT(called);

    called = false;
    auto signal2 = std::move(signal1);
    FCITX_ASSERT(connection.connected());
    // shouldn't use signal1() anymore, unless we want to assign to it again.
    FCITX_ASSERT(!called);
    signal2();
    FCITX_ASSERT(called);
    signal2.disconnectAll();
    FCITX_ASSERT(!connection.connected());
}

int main() {
    test_simple_signal();
    test_combiner();
    test_custom_combiner();
    test_destruct_order();
    test_connectable_object();
    test_reference();
    test_move();

    return 0;
}
