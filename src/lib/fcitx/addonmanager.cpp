/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include "addonmanager.h"
#include "addonloader.h"
#include "addonloader_p.h"
#include "fcitx-config/iniparser.h"

namespace fcitx {

class Addon {
public:
    Addon(RawConfig &config) { m_info.loadInfo(config); }

    const AddonInfo &info() const { return m_info; }

    bool isValid() const { return m_info.isValid(); }

    bool loaded() const { return !!m_instance; }

    AddonInstance *instance() { return m_instance.get(); }

    void load(AddonManagerPrivate *managerP);

private:
    AddonInfo m_info;
    std::unique_ptr<AddonInstance> m_instance;
};

class AddonManagerPrivate {
public:
    AddonManagerPrivate(AddonManager *q) : q_ptr(q), instance(nullptr) {}

    AddonManager *q_ptr;
    FCITX_DECLARE_PUBLIC(AddonManager);

    std::unordered_map<std::string, std::unique_ptr<Addon>> addons;
    std::unordered_map<std::string, std::unique_ptr<AddonLoader>> loaders;

    Instance *instance;
};

void Addon::load(AddonManagerPrivate *managerP) {
    if (!isValid()) {
        return;
    }
    auto &loaders = managerP->loaders;
    if (loaders.count(m_info.type())) {
        m_instance.reset(
            loaders[m_info.type()]->load(m_info, managerP->q_func()));
    }
}

AddonManager::AddonManager()
    : d_ptr(std::make_unique<AddonManagerPrivate>(this)) {}

AddonManager::~AddonManager() {}

void AddonManager::registerLoader(std::unique_ptr<AddonLoader> loader) {
    FCITX_D();
    // same loader shouldn't register twice
    if (d->loaders.count(loader->type())) {
        return;
    }
    d->loaders.emplace(loader->type(), std::move(loader));
}

void AddonManager::registerDefaultLoader(StaticAddonRegistry *registry) {
    registerLoader(std::make_unique<SharedLibraryLoader>());
    if (registry) {
        registerLoader(std::make_unique<StaticLibraryLoader>(registry));
    }
}

void AddonManager::load() {
    FCITX_D();
    StandardPath path;
    auto files = path.multiOpenAll(StandardPath::Type::Data, "fcitx5/addon",
                                   O_RDONLY, filter::Suffix(".conf"));
    for (const auto &file : files) {
        auto &files = file.second;
        RawConfig config;
        // reverse the order, so we end up parse user file at last.
        for (auto iter = files.rbegin(), end = files.rend(); iter != end;
             iter++) {
            auto fd = iter->first;
            readFromIni(config, fd);
        }
        auto addon = std::make_unique<Addon>(config);
        if (addon->isValid()) {
            d->addons[addon->info().name()] = std::move(addon);
        }
    }

    for (auto &item : d->addons) {
        auto &addon = *item.second;
        addon.load(d);
    }
}

AddonInstance *AddonManager::addon(const std::string &name) {
    FCITX_D();
    if (d->addons.count(name)) {
        return d->addons[name]->instance();
    }
    return nullptr;
}

Instance *AddonManager::instance() {
    FCITX_D();
    return d->instance;
}

void AddonManager::setInstance(Instance *instance) {
    FCITX_D();
    d->instance = instance;
}
}
