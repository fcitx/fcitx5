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

#include <cassert>
#include <fcitx-utils/intrusivelist.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>

using namespace fcitx;

struct Foo : public IntrusiveListNode {
    Foo(int d) : data(d) {}
    int data;
};

int main() {
    IntrusiveList<Foo> list;
    Foo a(1), b(2), c(3), d(4);
    list.push_back(a);
    list.push_back(b);
    list.push_back(c);
    list.push_back(d);

    std::vector<int> check = {1, 2, 3, 4};

    assert(list.size() == 4);

    int idx = 0;
    for (auto &f : list) {
        assert(f.data == check[idx]);
        idx++;
    }
    assert(idx == 4);

    list.pop_back();
    assert(list.size() == 3);
    idx = 0;
    for (auto &f : list) {
        assert(f.data == check[idx]);
        idx++;
    }
    assert(idx == 3);

    static_assert(std::is_same<std::iterator_traits<decltype(list)::iterator>::iterator_category,
                               std::bidirectional_iterator_tag>::value,
                  "Error");

    auto iter = std::find_if(list.begin(), list.end(), [](Foo &f) { return f.data == 2; });
    assert(iter != list.end());
    list.erase(iter);
    assert(list.size() == 2);
    assert(list.front().data == 1);
    assert(list.back().data == 3);

    assert(std::distance(list.begin(), list.end()) == 2);

    auto iter2 = list.insert(list.begin(), d);
    assert(iter2->data == 4);
    assert(list.size() == 3);

    list.insert(list.end(), b);
    assert(list.size() == 4);

    std::vector<int> check2 = {4, 1, 3, 2};
    idx = 0;
    for (auto &f : list) {
        assert(f.data == check2[idx]);
        idx++;
    }
    return 0;
}
