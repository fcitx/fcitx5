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

#include "fcitx-utils/log.h"
#include <algorithm>
#include <fcitx-utils/intrusivelist.h>
#include <iterator>
#include <vector>

using namespace fcitx;

struct Foo : public IntrusiveListNode {
    Foo(int d) : data(d) {}
    int data;
};

void test_regular() {
    IntrusiveList<Foo> list;
    Foo a(1), b(2), c(3), d(4);
    list.push_back(a);
    list.push_back(b);
    list.push_back(c);
    list.push_back(d);

    std::vector<int> check = {1, 2, 3, 4};

    FCITX_ASSERT(list.size() == 4);

    int idx = 0;
    for (auto &f : list) {
        FCITX_ASSERT(f.data == check[idx]);
        idx++;
    }
    FCITX_ASSERT(idx == 4);

    list.pop_back();
    FCITX_ASSERT(list.size() == 3);
    idx = 0;
    for (auto &f : list) {
        FCITX_ASSERT(f.data == check[idx]);
        idx++;
    }
    FCITX_ASSERT(idx == 3);

    static_assert(
        std::is_same<
            std::iterator_traits<decltype(list)::iterator>::iterator_category,
            std::bidirectional_iterator_tag>::value,
        "Error");

    auto iter = std::find_if(list.begin(), list.end(),
                             [](Foo &f) { return f.data == 2; });
    FCITX_ASSERT(iter != list.end());
    list.erase(iter);
    FCITX_ASSERT(list.size() == 2);
    FCITX_ASSERT(list.front().data == 1);
    FCITX_ASSERT(list.back().data == 3);

    FCITX_ASSERT(std::distance(list.begin(), list.end()) == 2);

    auto iter2 = list.insert(list.begin(), d);
    FCITX_ASSERT(iter2->data == 4);
    FCITX_ASSERT(list.size() == 3);

    list.insert(list.end(), b);
    FCITX_ASSERT(list.size() == 4);

    std::vector<int> check2 = {4, 1, 3, 2};
    idx = 0;
    for (auto &f : list) {
        FCITX_ASSERT(f.data == check2[idx]);
        idx++;
    }

    idx = 0;
    auto list2 = std::move(list);
    for (auto &f : list2) {
        FCITX_ASSERT(f.data == check2[idx]);
        idx++;
    }
}

void test_move() {
    {
        // empty to empty
        IntrusiveList<Foo> list;
        IntrusiveList<Foo> list2;
        list2 = std::move(list);
        FCITX_ASSERT(list2.size() == 0);
    }
    {
        // something to empty
        IntrusiveList<Foo> list;
        Foo a(1), b(2), c(3), d(4);
        list.push_back(a);
        list.push_back(b);
        list.push_back(c);
        list.push_back(d);
        IntrusiveList<Foo> list2;
        list2 = std::move(list);
        FCITX_ASSERT(a.isInList());
        FCITX_ASSERT(b.isInList());
        FCITX_ASSERT(c.isInList());
        FCITX_ASSERT(d.isInList());
        FCITX_ASSERT(list2.size() == 4);

        std::vector<int> check = {1, 2, 3, 4};
        size_t idx = 0;
        for (auto &f : list2) {
            FCITX_ASSERT(f.data == check[idx]);
            idx++;
        }
    }
    {
        // something to empty
        IntrusiveList<Foo> list;
        IntrusiveList<Foo> list2;
        Foo a(1), b(2), c(3), d(4);
        list2.push_back(a);
        list2.push_back(b);
        list2.push_back(c);
        list2.push_back(d);
        list2 = std::move(list);
        FCITX_ASSERT(!a.isInList());
        FCITX_ASSERT(!b.isInList());
        FCITX_ASSERT(!c.isInList());
        FCITX_ASSERT(!d.isInList());
        FCITX_ASSERT(list2.size() == 0);
    }
}

int main() {
    test_regular();
    test_move();
    return 0;
}
