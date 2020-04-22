//
// Copyright (C) 2020~2020 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
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
