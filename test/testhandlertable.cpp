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

#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/log.h"
#include <memory>
#include <unordered_set>

typedef std::function<void()> Callback;
using namespace fcitx;
static std::unordered_set<std::string> keys;

int main() {
    std::unique_ptr<HandlerTableEntry<Callback>> entry;
    {
        HandlerTable<Callback> table;
        entry = table.add([]() {});
        FCITX_ASSERT(table.size() == 1);
        entry.reset();
        FCITX_ASSERT(table.size() == 0);

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

        auto table2 = std::move(table);

        {
            FCITX_ASSERT(table2.size() == 6);
            for (auto &handler : table2.view()) {
                handler();
            }
            FCITX_ASSERT(table2.size() == 1);
        }
    }

    entry.reset();

    {
        MultiHandlerTable<std::string, Callback> table2(
            [](const std::string &key) {
                auto result = keys.insert(key);
                FCITX_ASSERT(result.second);
                return true;
            },
            [](const std::string &key) {
                auto result = keys.erase(key);
                FCITX_ASSERT(result);
            });
        entry = table2.add("ABC", []() {});
        FCITX_ASSERT(keys == decltype(keys){"ABC"});
        entry.reset();
        FCITX_ASSERT(keys == decltype(keys){});
        std::unique_ptr<HandlerTableEntry<Callback>> entries[] = {
            std::unique_ptr<HandlerTableEntry<Callback>>(
                table2.add("ABC", []() {})),
            std::unique_ptr<HandlerTableEntry<Callback>>(
                table2.add("ABC", []() {})),
        };
        FCITX_ASSERT(keys == decltype(keys){"ABC"});
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

        FCITX_ASSERT(keys2 == keys);

        FCITX_ASSERT(keys == (decltype(keys){"ABC", "DEF", "EFG"}));

        for (auto &e : entries) {
            e.reset(nullptr);
        }
        FCITX_ASSERT(keys == (decltype(keys){"DEF", "EFG"}));
    }

    return 0;
}
