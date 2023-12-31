/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "keydata.h"
#include <algorithm>
#include <cstddef>

namespace fcitx {
const UnicodeToKeySymTab &unicode_to_keysym_tab() {
    static const UnicodeToKeySymTab tab = []() {
        UnicodeToKeySymTab tab;
        for (size_t i = 0; i < tab.size(); i++) {
            tab[i] = keysym_to_unicode_tab[i];
        }
        std::stable_sort(tab.begin(), tab.end(), [](auto &lhs, auto &rhs) {
            return lhs.ucs < rhs.ucs;
        });
        return tab;
    }();

    return tab;
}
} // namespace fcitx