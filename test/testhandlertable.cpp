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

#include "fcitx-utils/handlertable.h"
#include <cassert>
#include <memory>
#include <unordered_set>

typedef std::function<void()> Callback;
using namespace fcitx;
static std::unordered_set<std::string> keys;

int main() {
    HandlerTableEntry<Callback> *entry;
    {
        HandlerTable<Callback> table;
        entry = table.add([]() {});
        delete entry;

        entry = table.add([]() {});
        std::unique_ptr<HandlerTableEntry<Callback>> entries[] = {
            std::unique_ptr<HandlerTableEntry<Callback>>(
                table.add([&entries]() {
                    // entries is member of lambda, and it will be gone if it's
                    // deleted
                    auto e = entries;
                    for (int i = 0; i < 5; i++) {
                        e[i].reset(nullptr);
                    }
                })),
            std::unique_ptr<HandlerTableEntry<Callback>>(table.add([]() {})),
            std::unique_ptr<HandlerTableEntry<Callback>>(table.add([]() {})),
            std::unique_ptr<HandlerTableEntry<Callback>>(table.add([]() {})),
            std::unique_ptr<HandlerTableEntry<Callback>>(table.add([]() {})),
        };

        {
            for (auto &handler : table.view()) {
                handler();
            }
        }
    }

    delete entry;

    {
        MultiHandlerTable<std::string, Callback> table2(
            [](const std::string &key) {
                auto result = keys.insert(key);
                assert(result.second);
            },
            [](const std::string &key) {
                auto result = keys.erase(key);
                assert(result);
            });
        entry = table2.add("ABC", []() {});
        assert(keys == decltype(keys){"ABC"});
        delete entry;
        assert(keys == decltype(keys){});
        std::unique_ptr<HandlerTableEntry<Callback>> entries[] = {
            std::unique_ptr<HandlerTableEntry<Callback>>(
                table2.add("ABC", []() {})),
            std::unique_ptr<HandlerTableEntry<Callback>>(
                table2.add("ABC", []() {})),
        };
        assert(keys == decltype(keys){"ABC"});
        std::unique_ptr<HandlerTableEntry<Callback>> entries2[] = {
            std::unique_ptr<HandlerTableEntry<Callback>>(
                table2.add("DEF", [&entries2]() { entries2[1].reset(); })),
            std::unique_ptr<HandlerTableEntry<Callback>>(
                table2.add("EFG", []() {})),
        };

        std::unordered_set<std::string> keys2;
        for (const auto &key : table2.keys()) {
            keys2.insert(key);
        }

        assert(keys2 == keys);

        assert(keys == (decltype(keys){"ABC", "DEF", "EFG"}));

        for (auto &e : entries) {
            e.reset(nullptr);
        }
        assert(keys == (decltype(keys){"DEF", "EFG"}));
    }

    return 0;
}
