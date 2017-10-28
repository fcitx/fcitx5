/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "spell.h"
#include "config.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonmanager.h"
#include "spell-custom.h"
#include "spell-enchant.h"
#include <fcntl.h>

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
        auto iter = backends_.find(backend);
        if (iter != backends_.end() && iter->second->checkDict(language)) {
            return iter;
        }
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

class SpellModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Spell(manager->instance());
    }
};
}

FCITX_ADDON_FACTORY(fcitx::SpellModuleFactory)
