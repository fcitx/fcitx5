/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_SPELL_SPELL_PUBLIC_H_
#define _FCITX_MODULES_SPELL_SPELL_PUBLIC_H_

#include <vector>
#include <fcitx/addoninstance.h>

namespace fcitx {
enum class SpellProvider { Presage, Custom, Enchant, Default = -1 };
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
FCITX_ADDON_DECLARE_FUNCTION(Spell, hintForDisplay,
                             std::vector<std::pair<std::string, std::string>>(
                                 const std::string &language,
                                 fcitx::SpellProvider provider,
                                 const std::string &word, size_t limit));

#endif // _FCITX_MODULES_SPELL_SPELL_PUBLIC_H_
