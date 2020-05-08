/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_MODULES_QUICKPHRASE_QUICKPHRASEPROVIDER_H_
#define _FCITX5_MODULES_QUICKPHRASE_QUICKPHRASEPROVIDER_H_

#include <functional>
#include <map>
#include <string>
#include "fcitx-utils/connectableobject.h"
#include "fcitx-utils/standardpath.h"
#include "quickphrase_public.h"

namespace fcitx {

class QuickPhraseProvider {
public:
    virtual ~QuickPhraseProvider() = default;
    virtual bool populate(InputContext *ic, const std::string &userInput,
                          QuickPhraseAddCandidateCallback addCandidate) = 0;
};

class BuiltInQuickPhraseProvider : public QuickPhraseProvider {
public:
    bool populate(InputContext *ic, const std::string &userInput,
                  QuickPhraseAddCandidateCallback addCandidate) override;
    void reloadConfig();

private:
    void load(StandardPathFile &file);
    std::multimap<std::string, std::string> map_;
};

class CallbackQuickPhraseProvider : public QuickPhraseProvider,
                                    public ConnectableObject {
public:
    bool populate(InputContext *ic, const std::string &userInput,
                  QuickPhraseAddCandidateCallback addCandidate) override;

    std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallback>>
    addCallback(QuickPhraseProviderCallback callback) {
        return callback_.add(callback);
    }

private:
    HandlerTable<QuickPhraseProviderCallback> callback_;
};

} // namespace fcitx

#endif // _FCITX5_MODULES_QUICKPHRASE_QUICKPHRASEPROVIDER_H_
