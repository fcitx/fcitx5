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
#include <unordered_set>
#include <fcntl.h>
#include <unistd.h>
#include "addonmanager.h"
#include "addonloader.h"
#include "addonloader_p.h"
#include "fcitx-config/iniparser.h"
#include "instance.h"

namespace fcitx {

class Addon {
public:
    Addon(RawConfig &config) : m_failed(false) { m_info.loadInfo(config); }

    const AddonInfo &info() const { return m_info; }

    bool isValid() const { return m_info.isValid() && !m_failed; }

    bool loaded() const { return !!m_instance; }

    AddonInstance *instance() { return m_instance.get(); }

    void load(AddonManagerPrivate *managerP);
    void setFailed(bool failed = true) { m_failed = failed; }

private:
    AddonInfo m_info;
    bool m_failed;
    std::unique_ptr<AddonInstance> m_instance;
};

enum class DependencyCheckStatus { Satisfied, Pending, PendingUpdateRequest, Failed };

class AddonManagerPrivate {
public:
    AddonManagerPrivate(AddonManager *q) : q_ptr(q), instance(nullptr) {}

    Addon *addon(const std::string &name) const {
        auto iter = addons.find(name);
        if (iter != addons.end()) {
            return iter->second.get();
        }
        return nullptr;
    }

    DependencyCheckStatus checkDependencies(const Addon &a) {
        auto &dependencies = a.info().dependencies();
        for (auto &dependency : dependencies) {
            Addon *dep = addon(dependency);
            if (!dep || !dep->isValid()) {
                return DependencyCheckStatus::Failed;
            }

            if (!dep->loaded()) {
                if (dep->info().onRequest() && requested.insert(dep->info().name()).second) {
                    return DependencyCheckStatus::PendingUpdateRequest;
                }
                return DependencyCheckStatus::Pending;
            }
        }
        auto &optionalDependencies = a.info().optionalDependencies();
        for (auto &dependency : optionalDependencies) {
            Addon *dep = addon(dependency);
            // if not available, don't bother load it
            if (!dep || !dep->isValid()) {
                continue;
            }

            // otherwise wait for it
            if (!dep->loaded()) {
                if (dep->info().onRequest() && requested.insert(dep->info().name()).second) {
                    return DependencyCheckStatus::PendingUpdateRequest;
                }
                return DependencyCheckStatus::Pending;
            }
        }

        return DependencyCheckStatus::Satisfied;
    }

    void loadAddons() {
        bool changed = false;
        do {
            changed = false;

            for (auto &item : addons) {
                changed = loadAddon(*item.second.get());
            }
        } while (changed);
    }

    bool loadAddon(Addon &addon) {
        if (addon.loaded() || !addon.isValid()) {
            return false;
        }
        if (addon.info().onRequest() && requested.count(addon.info().name()) == 0) {
            return false;
        }
        auto result = checkDependencies(addon);
        if (result == DependencyCheckStatus::Failed) {
            addon.setFailed();
        } else if (result == DependencyCheckStatus::Satisfied) {
            addon.load(this);
            if (addon.loaded()) {
                loadOrder.push_back(addon.info().name());
                return true;
            }
        } else if (result == DependencyCheckStatus::PendingUpdateRequest) {
            return true;
        }
        // here we are "pending" on others.
        return false;
    }

    AddonManager *q_ptr;
    FCITX_DECLARE_PUBLIC(AddonManager);

    std::unordered_map<std::string, std::unique_ptr<Addon>> addons;
    std::unordered_map<std::string, std::unique_ptr<AddonLoader>> loaders;
    std::unordered_set<std::string> requested;

    std::vector<std::string> loadOrder;

    Instance *instance;
};

void Addon::load(AddonManagerPrivate *managerP) {
    if (!isValid()) {
        return;
    }
    auto &loaders = managerP->loaders;
    if (loaders.count(m_info.type())) {
        m_instance.reset(loaders[m_info.type()]->load(m_info, managerP->q_func()));
    }
    if (!m_instance) {
        m_failed = true;
    }
}

AddonManager::AddonManager() : d_ptr(std::make_unique<AddonManagerPrivate>(this)) {}

AddonManager::~AddonManager() {
    FCITX_D();
    // reverse the unload order
    for (auto iter = d->loadOrder.rbegin(), end = d->loadOrder.rend(); iter != end; iter++) {
        d->addons.erase(*iter);
    }
}

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
    auto &path = StandardPath::global();
    auto files = path.multiOpenAll(StandardPath::Type::Data, "fcitx5/addon", O_RDONLY, filter::Suffix(".conf"));
    for (const auto &file : files) {
        auto &files = file.second;
        RawConfig config;
        // reverse the order, so we end up parse user file at last.
        for (auto iter = files.rbegin(), end = files.rend(); iter != end; iter++) {
            auto fd = iter->first;
            readFromIni(config, fd);
        }
        auto addon = std::make_unique<Addon>(config);
        if (addon->isValid()) {
            d->addons[addon->info().name()] = std::move(addon);
        }
    }

    d->loadAddons();
}

AddonInstance *AddonManager::addon(const std::string &name, bool load) {
    FCITX_D();
    auto addon = d->addon(name);
    if (!addon) {
        return nullptr;
    }
    if (addon->isValid() && !addon->loaded() && addon->info().onRequest() && load) {
        d->requested.insert(name);
        d->loadAddons();
    }
    return addon->instance();
}

const AddonInfo *AddonManager::addonInfo(const std::string &name) const {
    FCITX_D();
    auto addon = d->addon(name);
    if (addon && addon->isValid()) {
        return &addon->info();
    }
    return nullptr;
}

std::unordered_set<std::string> AddonManager::addonNames(AddonCategory category) {
    FCITX_D();
    std::unordered_set<std::string> result;
    for (auto &item : d->addons) {
        if (item.second->isValid() && item.second->info().category() == category) {
            result.insert(item.first);
        }
    }
    return result;
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
