/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "spell.h"
#include <fcntl.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonmanager.h"
#include "config.h"
#include "spell-custom.h"
#ifdef ENABLE_ENCHANT
#include "spell-enchant.h"
#endif

namespace fcitx {

Spell::Spell(Instance *instance) : instance_(instance) {
#ifdef ENABLE_ENCHANT
    backends_.emplace(SpellProvider::Enchant,
                      std::make_unique<SpellEnchant>(this));
#endif
    backends_.emplace(SpellProvider::Custom,
                      std::make_unique<SpellCustom>(this));

    reloadConfig();
}

Spell::~Spell() {}

void Spell::reloadConfig() { readAsIni(config_, "conf/spell.conf"); }

Spell::BackendMap::iterator Spell::findBackend(const std::string &language) {
    for (auto backend : config_.providerOrder.value()) {
        auto iter = findBackend(language, backend);
        if (iter != backends_.end()) {
            return iter;
        }
    }
    return backends_.end();
}

Spell::BackendMap::iterator Spell::findBackend(const std::string &language,
                                               SpellProvider provider) {
    auto iter = backends_.find(provider);
    if (iter != backends_.end() && iter->second->checkDict(language)) {
        return iter;
    }
    return backends_.end();
}

bool Spell::checkDict(const std::string &language) {
    auto iter = findBackend(language);
    return iter != backends_.end();
}

void Spell::addWord(const std::string &language, const std::string &word) {
    auto iter = findBackend(language);
    if (iter == backends_.end()) {
        return;
    }

    iter->second->addWord(language, word);
}

std::vector<std::string> Spell::hint(const std::string &language,
                                     const std::string &word, size_t limit) {
    auto iter = findBackend(language);
    if (iter == backends_.end()) {
        return {};
    }

    return iter->second->hint(language, word, limit);
}

std::vector<std::string> Spell::hintWithProvider(const std::string &language,
                                                 SpellProvider provider,
                                                 const std::string &word,
                                                 size_t limit) {
    auto iter = findBackend(language, provider);
    if (iter == backends_.end()) {
        return {};
    }

    return iter->second->hint(language, word, limit);
}

class SpellModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Spell(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::SpellModuleFactory)
