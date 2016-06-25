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
#ifndef _FCITX_IM_KEYBOARD_ISOCODES_H_
#define _FCITX_IM_KEYBOARD_ISOCODES_H_


#include <string>
#include <vector>
#include <unordered_map>
#include <libxml/parser.h>
#include "fcitx/misc_p.h"

namespace fcitx {

struct IsoCodes639Entry {
    std::string iso_639_2B_code;
    std::string iso_639_2T_code;
    std::string iso_639_1_code;
    std::string name;
};

class IsoCodes {
public:
    void read(const std::string &iso639, const std::string &iso3166);

    static void handleIsoCodes639StartElement(void *ctx, const xmlChar *name, const xmlChar **atts);
    static void handleIsoCodes3166StartElement(void *ctx, const xmlChar *name, const xmlChar **atts);

    const IsoCodes639Entry *entry(const std::string &name) const {
        auto entry = findValue(iso6392B, name);
        if (!entry) {
            entry = findValue(iso6392T, name);
        }
        if (!entry) {
            return nullptr;
        }
        return &iso639entires[*entry];
    }

private:
    void isoCodes639StartElement(const xmlChar *name, const xmlChar **atts);
    void isoCodes3166StartElement(const xmlChar *name, const xmlChar **atts);

    std::vector<IsoCodes639Entry> iso639entires;
    std::unordered_map<std::string, int> iso6392B;
    std::unordered_map<std::string, int> iso6392T;

    std::unordered_map<std::string, std::string> iso3166;
};
}

#endif // _FCITX_IM_KEYBOARD_ISOCODES_H_
