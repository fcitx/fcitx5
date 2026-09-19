/*
 * SPDX-FileCopyrightText: 2026 fcitx5 contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "casedata.h"
#include <algorithm>
#include <cstddef>

namespace fcitx {
const CasePairTab &case_pair_tab_by_lower() {
    static const CasePairTab tab = []() {
        CasePairTab tab;
        for (size_t i = 0; i < tab.size(); i++) {
            tab[i] = case_pair_tab[i];
        }
        std::ranges::stable_sort(
            tab, [](auto &lhs, auto &rhs) { return lhs.lower < rhs.lower; });
        return tab;
    }();

    return tab;
}

const CasePairTab &case_pair_tab_by_upper() {
    static const CasePairTab tab = []() {
        CasePairTab tab;
        for (size_t i = 0; i < tab.size(); i++) {
            tab[i] = case_pair_tab[i];
        }
        std::ranges::stable_sort(
            tab, [](auto &lhs, auto &rhs) { return lhs.upper < rhs.upper; });
        return tab;
    }();

    return tab;
}

} // namespace fcitx
