/*
 * Copyright (C) 2012~2012 by Yichao Yu
 * yyc1992@gmail.com
 * Copyright (C) 2017~2017 by CSSlayer
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

#include "spell-custom.h"
#include "fcitx-utils/cutf8.h"
#include "spell-custom-dict.h"

fcitx::SpellCustom::SpellCustom(fcitx::Spell *spell)
    : fcitx::SpellBackend(spell) {}

fcitx::SpellCustom::~SpellCustom() {}

void fcitx::SpellCustom::addWord(const std::string &, const std::string &) {
    // TODO
}

bool fcitx::SpellCustom::checkDict(const std::string &language) {
    return SpellCustomDict::checkDict(language);
}

bool fcitx::SpellCustom::loadDict(const std::string &language) {
    if (language_ == language) {
        return true;
    }

    auto dict = SpellCustomDict::requestDict(language);
    if (dict) {
        language_ = language;
        dict_.reset(dict);
        return true;
    }

    return false;
}

std::vector<std::string> fcitx::SpellCustom::hint(const std::string &language,
                                                  const std::string &str,
                                                  size_t limit) {
    if (!loadDict(language)) {
        return {};
    }
    return dict_->hint(str, limit);
}
