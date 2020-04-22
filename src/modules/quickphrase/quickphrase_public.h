//
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
#ifndef _FCITX_MODULES_QUICKPHRASE_QUICKPHRASE_PUBLIC_H_
#define _FCITX_MODULES_QUICKPHRASE_QUICKPHRASE_PUBLIC_H_

#include <functional>
#include <string>
#include <fcitx-utils/key.h>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance.h>
#include <fcitx/inputcontext.h>

namespace fcitx {

enum class QuickPhraseAction {
    Commit,
    TypeToBuffer,
    DigitSelection,
    AlphaSelection,
    NoneSelection,
    DoNothing
};

using QuickPhraseAddCandidateCallback = std::function<void(
    const std::string &, const std::string &, QuickPhraseAction action)>;
using QuickPhraseProviderCallback = std::function<bool(
    InputContext *ic, const std::string &, QuickPhraseAddCandidateCallback)>;

} // namespace fcitx

/// Trigger quickphrase, with following format:
/// description_text prefix_text
FCITX_ADDON_DECLARE_FUNCTION(QuickPhrase, trigger,
                             void(InputContext *ic, const std::string &text,
                                  const std::string &prefix,
                                  const std::string &str,
                                  const std::string &alt, const Key &key));

FCITX_ADDON_DECLARE_FUNCTION(
    QuickPhrase, addProvider,
    std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallback>>(
        QuickPhraseProviderCallback));

#endif // _FCITX_MODULES_QUICKPHRASE_QUICKPHRASE_PUBLIC_H_
