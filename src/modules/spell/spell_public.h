//
// Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_MODULES_SPELL_SPELL_PUBLIC_H_
#define _FCITX_MODULES_SPELL_SPELL_PUBLIC_H_

#include <vector>
#include <fcitx/addoninstance.h>

namespace fcitx {
enum class SpellProvider { Presage, Custom, Enchant };
}

FCITX_ADDON_DECLARE_FUNCTION(Spell, checkDict,
                             bool(const std::string &language));
FCITX_ADDON_DECLARE_FUNCTION(Spell, addWord,
                             void(const std::string &language,
                                  const std::string &word));
FCITX_ADDON_DECLARE_FUNCTION(
    Spell, hint,
    std::vector<std::string>(const std::string &language,
                             const std::string &word, size_t limit));
FCITX_ADDON_DECLARE_FUNCTION(
    Spell, hintWithProvider,
    std::vector<std::string>(const std::string &language,
                             fcitx::SpellProvider provider,
                             const std::string &word, size_t limit));

#endif // _FCITX_MODULES_SPELL_SPELL_PUBLIC_H_
