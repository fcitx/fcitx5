/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <memory>
#include <unordered_set>
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/log.h"

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
        FCITX_ASSERT(table.empty());

        entry = table.add([]() {});
        std::unique_ptr<HandlerTableEntry<Callback>> entries[] = {
            std::unique_ptr<HandlerTableEntry<Callback>>(
                table.add([&entries]() {
                    // entries is member of lambda, and it will be gone if it's
                    // deleted
                    auto *e = entries;
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
        FCITX_ASSERT(keys.empty());
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

    {
        MultiHandlerTable<std::string, int> table2(
            [](const std::string &key) {
                auto result = keys.insert(key);
                FCITX_ASSERT(result.second);
                return true;
            },
            [](const std::string &key) {
                auto result = keys.erase(key);
                FCITX_ASSERT(result);
            });
        auto entry = table2.add("ABC", 1);
        FCITX_ASSERT(keys == decltype(keys){"ABC"});
        std::unique_ptr<HandlerTableEntryBase> entries[] = {
            std::unique_ptr<HandlerTableEntryBase>(table2.add("ABC", 1)),
            std::unique_ptr<HandlerTableEntryBase>(table2.add("ABC", 1)),
        };
        FCITX_ASSERT(keys == decltype(keys){"ABC"});

        // Remove the 2nd and 3rd, there should be some key remaining.
        for (auto &e : entries) {
            e.reset(nullptr);
        }
        FCITX_ASSERT(keys == (decltype(keys){"ABC"}));
    }

    return 0;
}
