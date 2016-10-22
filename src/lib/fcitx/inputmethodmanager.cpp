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

#include "inputmethodmanager.h"
#include "addonmanager.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/standardpath.h"
#include "inputmethodconfig_p.h"
#include "inputmethodengine.h"
#include "instance.h"
#include "misc_p.h"
#include <cassert>
#include <fcntl.h>
#include <list>
#include <unistd.h>
#include <unordered_map>

namespace fcitx {

class InputMethodManagerPrivate {
public:
    InputMethodManagerPrivate(AddonManager *addonManager_) : addonManager_(addonManager_) {}

    AddonManager *addonManager_;
    std::string currentGroup_;
    std::list<std::string> groupOrder_;
    std::unordered_map<std::string, InputMethodGroup> groups_;
    std::unordered_map<std::string, InputMethodEntry> entries_;
    Instance *instance_ = nullptr;
    std::unique_ptr<HandlerTableEntry<EventHandler>> eventWatcher_;
};

bool checkEntry(const InputMethodEntry &entry, const std::unordered_set<std::string> &inputMethods) {
    return (entry.name().empty() || entry.uniqueName().empty() || entry.addon().empty() ||
            inputMethods.count(entry.addon()) == 0)
               ? false
               : true;
}

InputMethodManager::InputMethodManager(AddonManager *addonManager)
    : d_ptr(std::make_unique<InputMethodManagerPrivate>(addonManager)) {}

InputMethodManager::~InputMethodManager() {}

void InputMethodManager::load() {
    FCITX_D();

    auto inputMethods = d->addonManager_->addonNames(AddonCategory::InputMethod);
    auto &path = StandardPath::global();
    auto files = path.multiOpenAll(StandardPath::Type::Data, "fcitx5/inputmethod", O_RDONLY, filter::Suffix(".conf"));
    for (const auto &file : files) {
        auto &files = file.second;
        RawConfig config;
        // reverse the order, so we end up parse user file at last.
        for (auto iter = files.rbegin(), end = files.rend(); iter != end; iter++) {
            auto fd = iter->fd();
            readFromIni(config, fd);
        }

        InputMethodInfo imInfo;
        imInfo.load(config);
        InputMethodEntry entry = toInputMethodEntry(imInfo);
        if (checkEntry(entry, inputMethods) && d->entries_.count(entry.uniqueName()) == 0) {
            d->entries_.emplace(std::string(entry.uniqueName()), std::move(entry));
        }
    }
    for (const auto &addonName : inputMethods) {
        auto addonInfo = d->addonManager_->addonInfo(addonName);
        // on request input method should always provides entry with config file
        if (!addonInfo || addonInfo->onRequest()) {
            continue;
        }
        auto engine = static_cast<InputMethodEngine *>(d->addonManager_->addon(addonName));
        if (!engine) {
            continue;
        }
        auto newEntries = engine->listInputMethods();
        for (auto &newEntry : newEntries) {
            // ok we can't let you register something werid.
            if (checkEntry(newEntry, inputMethods) && newEntry.addon() == addonName &&
                d->entries_.count(newEntry.uniqueName()) == 0) {
                d->entries_.emplace(std::string(newEntry.uniqueName()), std::move(newEntry));
            }
        }
    }

    loadConfig();
}

void InputMethodManager::loadConfig() {
    FCITX_D();
    auto &path = StandardPath::global();
    auto file = path.open(StandardPath::Type::Config, "fcitx5/profile", O_RDONLY);
    RawConfig config;
    if (file.fd() >= 0) {
        readFromIni(config, file.fd());
    }
    InputMethodConfig imConfig;
    imConfig.load(config);

    d->groups_.clear();
    std::vector<std::string> tempOrder;
    if (imConfig.groups.value().size()) {
        auto &groupsConfig = imConfig.groups.value();
        for (auto &groupConfig : groupsConfig) {
            // group must have a name
            if (groupConfig.name.value().empty() || groupConfig.defaultLayout.value().empty()) {
                continue;
            }
            auto result = d->groups_.emplace(groupConfig.name.value(), InputMethodGroup(groupConfig.name.value()));
            tempOrder.push_back(groupConfig.name.value());
            auto &group = result.first->second;
            group.setDefaultLayout(groupConfig.defaultLayout.value());
            auto &items = groupConfig.items.value();
            for (auto &item : items) {
                group.inputMethodList().emplace_back(
                    std::move(InputMethodGroupItem(item.name.value()).setLayout(item.layout.value())));
            }
            group.setDefaultInputMethod(groupConfig.defaultInputMethod.value());
        }
    }

    if (d->groups_.size() == 0) {
        buildDefaultGroup();
    } else {
        setCurrentGroup(imConfig.currentGroup.value());
        if (imConfig.groupOrder.value().size()) {
            setGroupOrder(imConfig.groupOrder.value());
        } else {
            setGroupOrder(tempOrder);
        }
    }
}

void InputMethodManager::buildDefaultGroup() {
    FCITX_D();
    std::string name = _("Default");
    auto result = d->groups_.emplace(name, InputMethodGroup(name));
    auto &group = result.first->second;
    // FIXME
    group.inputMethodList().emplace_back(InputMethodGroupItem("fcitx-keyboard-us"));
    group.setDefaultInputMethod("");
    group.setDefaultLayout("us");
    setCurrentGroup(name);
    setGroupOrder({name});
}

int InputMethodManager::groupCount() const {
    FCITX_D();
    return d->groups_.size();
}

void InputMethodManager::setCurrentGroup(const std::string &groupName) {
    FCITX_D();
    if (std::any_of(d->groups_.begin(), d->groups_.end(),
                    [&groupName](const auto &group) { return group.second.name() == groupName; })) {
        d->currentGroup_ = groupName;
    } else {
        d->currentGroup_ = d->groups_.begin()->second.name();
    }
    // TODO Adjust group order
    // TODO, post event
}

const InputMethodGroup &InputMethodManager::currentGroup() const {
    FCITX_D();
    return d->groups_.find(d->currentGroup_)->second;
}

void InputMethodManager::setGroupOrder(const std::vector<std::string> &groupOrder) {
    FCITX_D();
    d->groupOrder_.clear();
    std::unordered_set<std::string> added;
    for (auto &groupName : groupOrder) {
        if (d->groups_.count(groupName)) {
            d->groupOrder_.push_back(groupName);
            added.insert(groupName);
        }
    }
    for (auto &p : d->groups_) {
        if (!added.count(p.first)) {
            d->groupOrder_.push_back(p.first);
        }
    }
    assert(d->groupOrder_.size() == d->groups_.size());
}

void InputMethodManager::save() {
    FCITX_D();
    InputMethodConfig config;
    std::vector<InputMethodGroupConfig> groups;
    config.currentGroup.setValue(d->currentGroup_);
    config.groupOrder.setValue(std::vector<std::string>{d->groupOrder_.begin(), d->groupOrder_.end()});

    for (auto &p : d->groups_) {
        auto &group = p.second;
        groups.emplace_back();
        auto &groupConfig = groups.back();
        groupConfig.name.setValue(group.name());
        groupConfig.defaultLayout.setValue(group.defaultLayout());
        groupConfig.defaultInputMethod.setValue(group.defaultInputMethod());
        std::vector<InputMethodGroupItemConfig> itemsConfig;
        for (auto &item : group.inputMethodList()) {
            itemsConfig.emplace_back();
            auto &itemConfig = itemsConfig.back();
            itemConfig.name.setValue(item.name());
            itemConfig.layout.setValue(item.layout());
        }

        groupConfig.items.setValue(std::move(itemsConfig));
    }
    config.groups.setValue(std::move(groups));

    RawConfig rawConfig;
    config.save(rawConfig);
    auto file = StandardPath::global().openUserTemp(StandardPath::Type::Config, "fcitx5/profile");
    if (file.fd() >= 0) {
        writeAsIni(rawConfig, file.fd());
    }
}

void InputMethodManager::setInstance(Instance *instance) {
    FCITX_D();
    d->instance_ = instance;
    d->eventWatcher_.reset(
        d->instance_->watchEvent(EventType::InputContextKeyEvent, EventWatcherPhase::InputMethod, [this](Event &event) {
            FCITX_D();
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto entry = d->instance_->inputMethodEntry(keyEvent.inputContext());
            if (!entry) {
                return;
            }
            auto engine = static_cast<InputMethodEngine *>(d->instance_->addonManager().addon(entry->addon()));
            if (!engine) {
                return;
            }
            engine->keyEvent(*entry, keyEvent);
        }));
}

const InputMethodEntry *InputMethodManager::entry(const std::string &name) const {
    FCITX_D();
    return findValue(d->entries_, name);
}
}
