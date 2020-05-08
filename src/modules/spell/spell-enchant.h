/*
 * SPDX-FileCopyrightText: 2012-2012 Yichao Yu <yyc1992@gmail.com>
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_SPELL_SPELL_ENCHANT_H_
#define _FCITX_MODULES_SPELL_SPELL_ENCHANT_H_

#include <enchant.h>
#include "spell.h"

namespace fcitx {

class SpellEnchant : public SpellBackend {
public:
    SpellEnchant(Spell *spell);
    ~SpellEnchant();

    bool checkDict(const std::string &language) override;
    void addWord(const std::string &language, const std::string &word) override;
    std::vector<std::string> hint(const std::string &language,
                                  const std::string &word,
                                  size_t limit) override;

private:
    bool loadDict(const std::string &language);
    std::unique_ptr<EnchantBroker, decltype(&enchant_broker_free)> broker_;
    std::unique_ptr<EnchantDict, std::function<void(EnchantDict *)>> dict_;
    std::string language_;
};
} // namespace fcitx

#endif // _FCITX_MODULES_SPELL_SPELL_ENCHANT_H_
