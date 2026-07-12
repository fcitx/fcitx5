/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "quickphrase.h"

#include <memory>
#include <utility>
#include "fcitx-config/iniparser.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/event.h"
#include "fcitx/instance.h"
#include "fcitx/tempmodemanager.h"

namespace fcitx {

QuickPhrase::QuickPhrase(Instance *instance)
    : instance_(instance), spellProvider_(this), tempMode_(this) {
    instance_->tempModeManager().registerTempMode(tempMode_);
    reloadConfig();
}

QuickPhrase::~QuickPhrase() {}

void QuickPhrase::reloadConfig() {
    builtinProvider_.reloadConfig();
    readAsIni(config_, "conf/quickphrase.conf");
}

std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallback>>
QuickPhrase::addProvider(QuickPhraseProviderCallback callback) {
    return callbackProvider_.addCallback(std::move(callback));
}

std::unique_ptr<HandlerTableEntry<QuickPhraseProviderCallbackV2>>
QuickPhrase::addProviderV2(QuickPhraseProviderCallbackV2 callback) {
    return callbackProvider_.addCallback(std::move(callback));
}

void QuickPhrase::trigger(InputContext *inputContext, const std::string &text,
                          const std::string &prefix, const std::string &str,
                          const std::string &alt, const Key &key) {
    tempMode_.trigger(inputContext, text, prefix, str, alt, key);
}

void QuickPhrase::setBuffer(InputContext *inputContext,
                            const std::string &text) {
    tempMode_.setBuffer(inputContext, text);
}

void QuickPhrase::setBufferWithRestoreCallback(
    InputContext *inputContext, const std::string &text,
    const std::string &original, QuickPhraseRestoreCallback callback) {
    tempMode_.setBufferWithRestoreCallback(inputContext, text, original,
                                           std::move(callback));
}

class QuickPhraseModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new QuickPhrase(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY_V2(quickphrase, fcitx::QuickPhraseModuleFactory)
