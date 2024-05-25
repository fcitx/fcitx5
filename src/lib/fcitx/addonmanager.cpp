/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "addonmanager.h"
#include <fcntl.h>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "fcitx-config/iniparser.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/semver.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/unixfd.h"
#include "addoninfo.h"
#include "addoninstance.h"
#include "addoninstance_p.h"
#include "addonloader.h"
#include "addonloader_p.h"
#include "config.h"
#include "instance.h"

namespace fcitx {

class Addon {
    friend class AddonManagerPrivate;

public:
    Addon(const std::string &name, RawConfig &config)
        : info_(name), failed_(false) {
        info_.load(config);
    }

    const AddonInfo &info() const { return info_; }

    bool isLoadable() const {
        return info_.isValid() && info_.isEnabled() && !failed_;
    }
    bool isValid() const { return info_.isValid() && !failed_; }

    bool loaded() const { return !!instance_; }

    AddonInstance *instance() { return instance_.get(); }

    void setFailed(bool failed = true) { failed_ = failed; }
    void setOverrideEnabled(OverrideEnabled overrideEnabled) {
        info_.setOverrideEnabled(overrideEnabled);
    }

private:
    AddonInfo info_;
    bool failed_;
    std::unique_ptr<AddonInstance> instance_;
};

enum class DependencyCheckStatus {
    Satisfied,
    Pending,
    PendingUpdateRequest,
    Failed
};

class AddonManagerPrivate {
public:
    AddonManagerPrivate() {}

    Addon *addon(const std::string &name) const {
        auto iter = addons_.find(name);
        if (iter != addons_.end()) {
            return iter->second.get();
        }
        return nullptr;
    }

    DependencyCheckStatus checkDependencies(const Addon &a) {
        const auto &dependencies = a.info().dependenciesWithVersion();
        for (const auto &[dependency, depVersion] : dependencies) {
            if (dependency == "core") {
                if (depVersion <= version_) {
                    continue;
                }
                return DependencyCheckStatus::Failed;
            }
            Addon *dep = addon(dependency);
            if (!dep || !dep->isLoadable()) {
                return DependencyCheckStatus::Failed;
            }

            if (depVersion > dep->info().version()) {
                return DependencyCheckStatus::Failed;
            }

            if (!dep->loaded()) {
                if (dep->info().onDemand() &&
                    requested_.insert(dep->info().uniqueName()).second) {
                    return DependencyCheckStatus::PendingUpdateRequest;
                }
                return DependencyCheckStatus::Pending;
            }
        }
        const auto &optionalDependencies = a.info().optionalDependencies();
        for (const auto &dependency : optionalDependencies) {
            Addon *dep = addon(dependency);
            // if not available, don't bother load it
            if (!dep || !dep->isLoadable()) {
                continue;
            }

            // otherwise wait for it
            if (!dep->loaded() && !dep->info().onDemand()) {
                return DependencyCheckStatus::Pending;
            }
        }

        return DependencyCheckStatus::Satisfied;
    }

    void loadAddons(AddonManager *q_ptr) {
        if (instance_ && instance_->exiting()) {
            return;
        }
        if (inLoadAddons_) {
            throw std::runtime_error("loadAddons is not reentrant, do not call "
                                     "addon(.., true) in constructor of addon");
        }
        inLoadAddons_ = true;
        bool changed = false;
        do {
            changed = false;

            for (auto &item : addons_) {
                changed |= loadAddon(q_ptr, *item.second);
                // Exit if addon request it.
                if (instance_ && instance_->exiting()) {
                    changed = false;
                    break;
                }
            }
        } while (changed);
        inLoadAddons_ = false;
    }

    bool loadAddon(AddonManager *q_ptr, Addon &addon) {
        if (unloading_) {
            return false;
        }

        if (addon.loaded() || !addon.isLoadable()) {
            return false;
        }
        if (addon.info().onDemand() &&
            requested_.count(addon.info().uniqueName()) == 0) {
            return false;
        }
        auto result = checkDependencies(addon);
        FCITX_DEBUG() << "Call loadAddon() with " << addon.info().uniqueName()
                      << " checkDependencies() returns "
                      << static_cast<int>(result)
                      << " Dep: " << addon.info().dependenciesWithVersion()
                      << " OptDep: "
                      << addon.info().optionalDependenciesWithVersion();
        if (result == DependencyCheckStatus::Failed) {
            addon.setFailed();
        } else if (result == DependencyCheckStatus::Satisfied) {
            realLoad(q_ptr, addon);
            if (addon.loaded()) {
                loadOrder_.push_back(addon.info().uniqueName());
                return true;
            }
        } else if (result == DependencyCheckStatus::PendingUpdateRequest) {
            return true;
        }
        // here we are "pending" on others.
        return false;
    }

    void realLoad(AddonManager *q_ptr, Addon &addon) {
        if (!addon.isLoadable()) {
            return;
        }

        if (auto *loader = findValue(loaders_, addon.info().type())) {
            addon.instance_.reset((*loader)->load(addon.info(), q_ptr));
        } else {
            FCITX_ERROR() << "Failed to find addon loader for: "
                          << addon.info().type();
        }
        if (!addon.instance_) {
            addon.setFailed(true);
            FCITX_INFO() << "Could not load addon "
                         << addon.info().uniqueName();
        } else {
            addon.instance_->d_func()->addonInfo_ = &(addon.info());
            FCITX_INFO() << "Loaded addon " << addon.info().uniqueName();
        }
    }

    std::string addonConfigDir_ = "addon";

    bool unloading_ = false;
    bool inLoadAddons_ = false;

    std::unordered_map<std::string, std::unique_ptr<Addon>> addons_;
    std::unordered_map<std::string, std::unique_ptr<AddonLoader>> loaders_;
    std::unordered_set<std::string> requested_;

    std::vector<std::string> loadOrder_;

    Instance *instance_ = nullptr;
    EventLoop *eventLoop_ = nullptr;
    int64_t timestamp_ = 0;
    const SemanticVersion version_ =
        SemanticVersion::parse(FCITX_VERSION_STRING).value();

    std::unordered_map<std::string, std::vector<std::string>> options_;
};

AddonManager::AddonManager() : d_ptr(std::make_unique<AddonManagerPrivate>()) {}

AddonManager::AddonManager(const std::string &addonConfigDir) : AddonManager() {
    FCITX_D();
    d->addonConfigDir_ = addonConfigDir;
}

AddonManager::~AddonManager() { unload(); }

void AddonManager::registerLoader(std::unique_ptr<AddonLoader> loader) {
    FCITX_D();
    // same loader shouldn't register twice
    if (d->loaders_.count(loader->type())) {
        return;
    }
    d->loaders_.emplace(loader->type(), std::move(loader));
}

void AddonManager::unregisterLoader(const std::string &name) {
    FCITX_D();
    d->loaders_.erase(name);
}

void AddonManager::registerDefaultLoader(StaticAddonRegistry *registry) {
    registerLoader(std::make_unique<SharedLibraryLoader>());
    if (registry) {
        registerLoader(std::make_unique<StaticLibraryLoader>(registry));
    }
}

void AddonManager::load(const std::unordered_set<std::string> &enabled,
                        const std::unordered_set<std::string> &disabled) {
    FCITX_D();
    const auto &path = StandardPath::global();
    d->timestamp_ =
        path.timestamp(StandardPath::Type::PkgData, d->addonConfigDir_);
    auto fileNames = path.locate(StandardPath::Type::PkgData,
                                 d->addonConfigDir_, filter::Suffix(".conf"));
    bool enableAll = enabled.count("all");
    bool disableAll = disabled.count("all");
    for (const auto &[fileName, fullName] : fileNames) {
        // remove .conf
        std::string name = fileName.substr(0, fileName.size() - 5);
        if (name == "core") {
            FCITX_ERROR() << "\"core\" is not a valid addon name.";
            continue;
        }
        if (d->addons_.count(name)) {
            continue;
        }

        RawConfig config;
        UnixFD fd = UnixFD::own(open(fullName.c_str(), O_RDONLY));
        readFromIni(config, fd.fd());

        // override configuration
        auto addon = std::make_unique<Addon>(name, config);
        if (addon->isValid()) {
            if (enableAll || enabled.count(name)) {
                addon->setOverrideEnabled(OverrideEnabled::Enabled);
            } else if (disableAll || disabled.count(name)) {
                addon->setOverrideEnabled(OverrideEnabled::Disabled);
            }
            d->addons_[addon->info().uniqueName()] = std::move(addon);
        }
    }

    d->loadAddons(this);
}

void AddonManager::unload() {
    FCITX_D();
    if (d->unloading_) {
        return;
    }
    d->unloading_ = true;
    // reverse the unload order
    for (auto iter = d->loadOrder_.rbegin(), end = d->loadOrder_.rend();
         iter != end; iter++) {
        FCITX_INFO() << "Unloading addon " << *iter;
        d->addons_.erase(*iter);
    }
    d->loadOrder_.clear();
    d->requested_.clear();
    d->unloading_ = false;
}

void AddonManager::saveAll() {
    FCITX_D();
    if (d->unloading_) {
        return;
    }
    // reverse the unload order
    for (auto iter = d->loadOrder_.rbegin(), end = d->loadOrder_.rend();
         iter != end; iter++) {
        if (auto *addonInst = addon(*iter)) {
            addonInst->save();
        }
    }
}

AddonInstance *AddonManager::addon(const std::string &name, bool load) {
    FCITX_D();
    auto *addon = d->addon(name);
    if (!addon) {
        return nullptr;
    }
    if (addon->isLoadable() && !addon->loaded() && addon->info().onDemand() &&
        load) {
        d->requested_.insert(name);
        d->loadAddons(this);
    }
    return addon->instance();
}

const AddonInfo *AddonManager::addonInfo(const std::string &name) const {
    FCITX_D();
    auto *addon = d->addon(name);
    if (addon && addon->isValid()) {
        return &addon->info();
    }
    return nullptr;
}

AddonInstance *AddonManager::lookupAddon(const std::string &name) const {
    FCITX_D();
    auto *addon = d->addon(name);
    return addon ? addon->instance() : nullptr;
}

const std::vector<std::string> &AddonManager::loadedAddonNames() const {
    FCITX_D();
    return d->loadOrder_;
}

std::unordered_set<std::string>
AddonManager::addonNames(AddonCategory category) {
    FCITX_D();
    std::unordered_set<std::string> result;
    for (auto &item : d->addons_) {
        if (item.second->isValid() &&
            item.second->info().category() == category) {
            result.insert(item.first);
        }
    }
    return result;
}

Instance *AddonManager::instance() {
    FCITX_D();
    return d->instance_;
}

void AddonManager::setInstance(Instance *instance) {
    FCITX_D();
    d->instance_ = instance;
    d->eventLoop_ = &instance->eventLoop();
}

void AddonManager::setEventLoop(EventLoop *eventLoop) {
    FCITX_D();
    d->eventLoop_ = eventLoop;
}

EventLoop *AddonManager::eventLoop() {
    FCITX_D();
    return d->eventLoop_;
}

const SemanticVersion &AddonManager::version() const {
    FCITX_D();
    return d->version_;
}

bool AddonManager::checkUpdate() const {
    FCITX_D();
    auto timestamp = StandardPath::global().timestamp(
        StandardPath::Type::PkgData, d->addonConfigDir_);
    return timestamp > d->timestamp_;
}

void AddonManager::setAddonOptions(
    std::unordered_map<std::string, std::vector<std::string>> options) {
    FCITX_D();
    d->options_ = std::move(options);
}

std::vector<std::string> AddonManager::addonOptions(const std::string &name) {
    FCITX_D();
    if (auto *options = findValue(d->options_, name)) {
        return *options;
    }
    return {};
}

} // namespace fcitx
