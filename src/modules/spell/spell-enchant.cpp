/*
 * SPDX-FileCopyrightText: 2012-2012 Yichao Yu <yyc1992@gmail.com>
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include <dlfcn.h>
#include <stdexcept>
#include <enchant.h>
#include "spell-enchant.h"

namespace fcitx {

SpellEnchant::SpellEnchant(Spell *spell)
    : SpellBackend(spell), broker_(enchant_broker_init()),
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
    if (!suggestions) {
        return {};
    }

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

    auto *dict = enchant_broker_request_dict(broker_.get(), language.c_str());
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
        enchant_dict_add(dict_.get(), word.c_str(), word.size());
    }
}

bool SpellEnchant::checkDict(const std::string &language) {
    return !!enchant_broker_dict_exists(broker_.get(), language.c_str());
}
} // namespace fcitx
