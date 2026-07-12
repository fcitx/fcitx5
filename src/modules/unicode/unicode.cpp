/*
 * SPDX-FileCopyrightText: 2012-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "unicode.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/tempmodemanager.h"

namespace fcitx {

Unicode::Unicode(Instance *instance) : instance_(instance), tempMode_(this) {
    KeySym syms[] = {
        FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
        FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
    };

    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym, KeyState::Alt);
    }

    instance_->tempModeManager().registerTempMode(tempMode_);
    reloadConfig();
}

Unicode::~Unicode() {}

bool Unicode::trigger(InputContext *inputContext) {
    return tempMode_.trigger(inputContext);
}

class UnicodeModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Unicode(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY_V2(unicode, fcitx::UnicodeModuleFactory);
