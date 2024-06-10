/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_IM_KEYBOARD_ISOCODES_H_
#define _FCITX_IM_KEYBOARD_ISOCODES_H_

#include <string>
#include <unordered_map>
#include <vector>
#include "fcitx-utils/misc_p.h"

namespace fcitx {

struct IsoCodes639Entry {
    std::string iso_639_2B_code;
    std::string iso_639_2T_code;
    std::string iso_639_1_code;
    std::string name;
};

class IsoCodes {
    friend class IsoCodes639Parser;
    friend class IsoCodes3166Parser;

public:
    void read(const std::string &iso639, const std::string &iso3166);

    const IsoCodes639Entry *entry(const std::string &name) const {
        const auto *entry = findValue(iso6392B, name);
        if (!entry) {
            entry = findValue(iso6392T, name);
        }
        if (!entry) {
            return nullptr;
        }
        return &iso639entires[*entry];
    }

private:
    std::vector<IsoCodes639Entry> iso639entires;
    std::unordered_map<std::string, int> iso6392B;
    std::unordered_map<std::string, int> iso6392T;

    std::unordered_map<std::string, std::string> iso3166;
};
} // namespace fcitx

#endif // _FCITX_IM_KEYBOARD_ISOCODES_H_
