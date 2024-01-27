/*
 * SPDX-FileCopyrightText: 2012-2012 Yichao Yu <yyc1992@gmail.com>
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "spell-enchant.h"
#include <time.h>
#include <stdexcept>
#include <enchant.h>
#include "fcitx/misc_p.h"

namespace fcitx {
namespace {
enum class SpellType { AllLower, Mixed, FirstUpper, AllUpper };

SpellType guessSpellType(const std::string &input) {
    if (input.size() <= 1) {
        if (charutils::isupper(input[0])) {
            return SpellType::FirstUpper;
        }
        return SpellType::AllLower;
    }

    if (std::all_of(input.begin(), input.end(),
                    [](char c) { return charutils::isupper(c); })) {
        return SpellType::AllUpper;
    }

    if (std::all_of(input.begin() + 1, input.end(),
                    [](char c) { return charutils::islower(c); })) {
        if (charutils::isupper(input[0])) {
            return SpellType::FirstUpper;
        }
        return SpellType::AllLower;
    }

    return SpellType::Mixed;
}

std::string formatWord(const std::string &input, SpellType type) {
    if (type == SpellType::Mixed || type == SpellType::AllLower) {
        return input;
    }
    if (guessSpellType(input) != SpellType::AllLower) {
        return input;
    }
    std::string result;
    if (type == SpellType::AllUpper) {
        result.reserve(input.size());
        std::transform(input.begin(), input.end(), std::back_inserter(result),
                       charutils::toupper);
    } else {
        // FirstUpper
        result = input;
        if (!result.empty()) {
            result[0] = charutils::toupper(result[0]);
        }
    }
    return result;
}

} // namespace

template <typename Callback>
auto foreachLanguage(const std::string &lang, const std::string &systemLanguage,
                     const Callback &callback) {
    static const std::unordered_map<std::string, std::vector<std::string>>
        fallbackLanguage = {
            {"de", {"de_DE"}}, {"el", {"el_GR"}}, {"he", {"he_IL"}},
            {"fr", {"fr_FR"}}, {"hu", {"hu_HU"}}, {"it", {"it_IT"}},
            {"it", {"nl_NL"}}, {"pl", {"pl_PL"}}, {"ro", {"ro_RO"}},
            {"ru", {"ru_UR"}}, {"es", {"es_ES"}},
        };

    std::vector<std::string> langList;
    if (stringutils::startsWith(systemLanguage,
                                stringutils::concat(lang, "_")) &&
        lang != systemLanguage) {
        langList.push_back(systemLanguage);
    };
    langList.push_back(lang);
    if (auto *fallback = findValue(fallbackLanguage, lang)) {
        for (const auto &fallbackLang : *fallback) {
            if (fallbackLang != langList[0]) {
                langList.push_back(fallbackLang);
            }
        }
    }
    decltype(callback(lang)) result{};
    for (const auto &lang : langList) {
        result = callback(lang);
        if (result) {
            break;
        }
    }
    return result;
}

SpellEnchant::SpellEnchant(Spell *spell)
    : SpellBackend(spell), broker_(enchant_broker_init()),
      dict_(nullptr,
            [this](EnchantDict *dict) {
                enchant_broker_free_dict(broker_.get(), dict);
            }),
      systemLanguage_(stripLanguage(getCurrentLanguage())) {
    if (!broker_) {
        throw std::runtime_error("Init enchant failed");
    }
}

SpellEnchant::~SpellEnchant() {}

std::vector<std::pair<std::string, std::string>>
SpellEnchant::hint(const std::string &language, const std::string &word,
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

    std::vector<std::pair<std::string, std::string>> result;
    number = number > limit ? limit : number;
    result.reserve(number);
    auto spellType = guessSpellType(word);
    for (size_t i = 0; i < number; i++) {
        std::string hintWord = suggestions[i];
        hintWord = formatWord(hintWord, spellType);
        result.emplace_back(hintWord, hintWord);
    }

    enchant_dict_free_string_list(dict_.get(), suggestions);
    return result;
}

bool SpellEnchant::loadDict(const std::string &language) {
    if (language_ == language) {
        return true;
    }

    auto *dict = foreachLanguage(
        language, systemLanguage_, [this](const std::string &lang) {
            return enchant_broker_request_dict(broker_.get(), lang.c_str());
        });
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
    return foreachLanguage(
        language, systemLanguage_, [this](const std::string &lang) {
            return !!enchant_broker_dict_exists(broker_.get(), lang.c_str());
        });
}
} // namespace fcitx
