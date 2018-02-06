//
// Copyright (C) 2012~2012 by Yichao Yu
// yyc1992@gmail.com
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_MODULES_SPELL_SPELL_ENCHANT_H_
#define _FCITX_MODULES_SPELL_SPELL_ENCHANT_H_

#include "spell.h"
#include <enchant.h>

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
