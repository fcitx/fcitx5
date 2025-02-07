/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_MODULES_QUICKPHRASE_QUICKPHRASEPROVIDER_H_
#define _FCITX5_MODULES_QUICKPHRASE_QUICKPHRASEPROVIDER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include "fcitx-utils/connectableobject.h"
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/misc.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "quickphrase_public.h"

namespace fcitx {

class QuickPhrase;

class QuickPhraseProvider {
public:
    virtual ~QuickPhraseProvider() = default;
    virtual bool
    populate(InputContext *ic, const std::string &userInput,
             const QuickPhraseAddCandidateCallbackV2 &addCandidate) = 0;
};

class BuiltInQuickPhraseProvider : public QuickPhraseProvider {
public:
    bool
    populate(InputContext *ic, const std::string &userInput,
             const QuickPhraseAddCandidateCallbackV2 &addCandidate) override;
    void reloadConfig();

private:
    void load(int fd);
    std::multimap<std::string, std::string> map_;
};

class SpellQuickPhraseProvider : public QuickPhraseProvider {
public:
    SpellQuickPhraseProvider(QuickPhrase *parent);
    FCITX_ADDON_DEPENDENCY_LOADER(spell, instance_->addonManager());

    bool
    populate(InputContext *ic, const std::string &userInput,
             const QuickPhraseAddCandidateCallbackV2 &addCandidate) override;

private:
    QuickPhrase *parent_;
    Instance *instance_;
};

class CallbackQuickPhraseProvider : public QuickPhraseProvider,
                                    public ConnectableObject {
public:
    bool
    populate(InputContext *ic, const std::string &userInput,
             const QuickPhraseAddCandidateCallbackV2 &addCandidate) override;

    std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallback>>
    addCallback(QuickPhraseProviderCallback callback) {
        return callback_.add(std::move(callback));
    }

    std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallbackV2>>
    addCallback(QuickPhraseProviderCallbackV2 callback) {
        return callbackV2_.add(std::move(callback));
    }

private:
    HandlerTable<QuickPhraseProviderCallback> callback_;
    HandlerTable<QuickPhraseProviderCallbackV2> callbackV2_;
};

} // namespace fcitx

#endif // _FCITX5_MODULES_QUICKPHRASE_QUICKPHRASEPROVIDER_H_
