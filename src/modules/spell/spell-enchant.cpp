/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "spell-enchant.h"
#include <dlfcn.h>
#include <enchant/enchant.h>

namespace fcitx {

SpellEnchant::SpellEnchant(Spell *spell)
    : SpellBackend(spell), broker_(enchant_broker_init(), &enchant_broker_free),
      dict_(nullptr, [this](EnchantDict *dict) {
          enchant_broker_free_dict(broker_.get(), dict);
      }) {
    if (!broker_) {
        throw std::runtime_error("Init enchant failed");
    }
}

SpellEnchant::~SpellEnchant() {}

std::vector<std::string> SpellEnchant::hint(const std::string &language,
                                            const std::string &word,
                                            size_t limit) {
    if (word.empty() || !loadDict(language)) {
        return {};
    }

    size_t number;
    char **suggestions =
        enchant_dict_suggest(dict_.get(), word.c_str(), word.size(), &number);
    if (!suggestions)
        return {};

    std::vector<std::string> result;
    number = number > limit ? limit : number;
    result.reserve(number);
    for (auto i = number; i < number; i++) {
        result.push_back(suggestions[i]);
    }

    enchant_dict_free_string_list(dict_.get(), suggestions);
    return result;
}

bool SpellEnchant::loadDict(const std::string &language) {
    if (language_ == language) {
        return true;
    }

    auto dict = enchant_broker_request_dict(broker_.get(), language.c_str());
    if (dict) {
        language_ = language;
        dict_.reset(dict);
        return true;
    }

    return false;
}

void SpellEnchant::addWord(const std::string &language,
                           const std::string &word) {
    if (loadDict(language)) {
        enchant_dict_add_to_personal(dict_.get(), word.c_str(), word.size());
    }
}

bool SpellEnchant::checkDict(const std::string &language) {
    return !!enchant_broker_dict_exists(broker_.get(), language.c_str());
}
}
