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

#include "fcitx-utils/signals.h"
#include <cassert>

void test_simple_signal() {
    bool called = false;
    fcitx::Signal<void()> signal1;
    auto connection = signal1.connect([&called]() {
        called = true;
        return;
    });
    assert(connection.connected());
    assert(!called);
    signal1();
    called = false;
    auto connection2 = connection;
    assert(connection.connected());
    assert(connection2.connected());

    connection.disconnect();
    assert(!connection.connected());
    assert(!connection2.connected());
    signal1();
    assert(!called);
}

void test_combiner() {
    fcitx::Signal<int(int)> signal1;
    auto connection = signal1.connect([](int i) { return i; });
    assert(signal1(1) == 1);
    auto connection2 = signal1.connect([](int) { return -1; });
    assert(signal1(2) == -1);
}

class MaxInteger {
public:
    MaxInteger() : m_initial(INT32_MIN) {}

    template <typename InputIterator>
    int operator()(InputIterator begin, InputIterator end) {
        int v = m_initial;
        for (; begin != end; begin++) {
            int newv = *begin;
            if (newv > v) {
                v = newv;
            }
        }
        return v;
    }

private:
    int m_initial;
};

void test_custom_combiner() {
    fcitx::Signal<int(int), MaxInteger> signal1;
    auto connection = signal1.connect([](int i) { return i; });
    assert(signal1(1) == 1);
    auto connection2 = signal1.connect([](int) { return -1; });
    assert(signal1(2) == 2);
    connection.disconnect();
    assert(signal1(2) == -1);
}

void test_destruct_order() {
    fcitx::Connection connection;
    assert(!connection.connected());
    {
        fcitx::Signal<void()> signal1;
        connection = signal1.connect([]() {});

        assert(connection.connected());
    }
    assert(!connection.connected());
    connection.disconnect();
}

int main() {
    test_simple_signal();
    test_combiner();
    test_custom_combiner();
    test_destruct_order();
    return 0;
}
