/*
 * SPDX-FileCopyrightText: 2012-2012 Yichao Yu <yyc1992@gmail.com>
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "spell-custom.h"
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

    auto *dict = SpellCustomDict::requestDict(language);
    if (dict) {
        language_ = language;
        dict_.reset(dict);
        return true;
    }

    return false;
}

std::vector<std::pair<std::string, std::string>>
fcitx::SpellCustom::hint(const std::string &language, const std::string &str,
                         size_t limit) {
    if (!loadDict(language)) {
        return {};
    }
    return dict_->hint(str, limit);
}
