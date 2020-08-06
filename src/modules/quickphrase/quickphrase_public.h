/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
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
using QuickPhraseProviderCallback =
    std::function<bool(InputContext *ic, const std::string &,
                       const QuickPhraseAddCandidateCallback &)>;

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
