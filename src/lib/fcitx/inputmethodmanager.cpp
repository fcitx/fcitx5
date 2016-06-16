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

#include <cassert>
#include <list>
#include <unistd.h>
#include "inputmethodmanager.h"
#include "inputmethodengine.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-config/iniparser.h"
#include "inputmethodconfig_p.h"
#include "addonmanager.h"
#include <fcntl.h>
#include <unordered_map>
#include <iostream>
#include "misc_p.h"

namespace fcitx {

class InputMethodManagerPrivate {
public:
    InputMethodManagerPrivate(AddonManager *addonManager_) : addonManager(addonManager_) {}

    AddonManager *addonManager;
    std::string currentGroup;
    std::list<std::string> groupOrder;
    std::unordered_map<std::string, InputMethodGroup> groups;
    std::unordered_map<std::string, InputMethodEntry> entries;
    Instance *instance = nullptr;
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

    auto inputMethods = d->addonManager->addonNames(AddonCategory::InputMethod);
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
        if (checkEntry(entry, inputMethods) && d->entries.count(entry.uniqueName()) == 0) {
            d->entries.emplace(std::string(entry.uniqueName()), std::move(entry));
        }
    }
    for (const auto &addonName : inputMethods) {
        auto addonInfo = d->addonManager->addonInfo(addonName);
        // on request input method should always provides entry with config file
        if (!addonInfo || addonInfo->onRequest()) {
            continue;
        }
        auto engine = static_cast<InputMethodEngine *>(d->addonManager->addon(addonName));
        if (!engine) {
            continue;
        }
        auto newEntries = engine->listInputMethods();
        for (auto &newEntry : newEntries) {
            // ok we can't let you register something werid.
            if (checkEntry(newEntry, inputMethods) && newEntry.addon() == addonName &&
                d->entries.count(newEntry.uniqueName()) == 0) {
                d->entries.emplace(std::string(newEntry.uniqueName()), std::move(newEntry));
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

    d->groups.clear();
    if (imConfig.groups.value().size()) {
        auto &groupsConfig = imConfig.groups.value();
        for (auto &groupConfig : groupsConfig) {
            // group must have a name
            if (groupConfig.name.value().empty()) {
                continue;
            }
            auto result = d->groups.emplace(groupConfig.name.value(), InputMethodGroup(groupConfig.name.value()));
            auto &group = result.first->second;
            group.setDefaultLayout(groupConfig.defaultLayout.value());
            auto &items = groupConfig.items.value();
            for (auto &item : items) {
                group.inputMethodList().emplace_back(
                    std::move(InputMethodGroupItem(item.name.value()).setLayout(item.layout.value())));
            }
            group.setDefaultInputMethod(groupConfig.defaultInputMethod.value());
        }
        setCurrentGroup(imConfig.currentGroup.value());

    } else {
        buildDefaultGroup();
    }
}

void InputMethodManager::buildDefaultGroup() {
    FCITX_D();
    std::string name = _("Default");
    auto result = d->groups.emplace(name, InputMethodGroup(name));
    auto &group = result.first->second;
    // FIXME
    group.inputMethodList().emplace_back(InputMethodGroupItem("fcitx-keyboard-us"));
    group.setDefaultInputMethod("");
    setCurrentGroup(name);
    setGroupOrder({name});
}

int InputMethodManager::groupCount() const {
    FCITX_D();
    return d->groups.size();
}

void InputMethodManager::setCurrentGroup(const std::string &groupName) {
    FCITX_D();
    if (std::any_of(d->groups.begin(), d->groups.end(), [&groupName](const auto &group) { return group.second.name() == groupName; })) {
        d->currentGroup = groupName;
    } else {
        d->currentGroup = d->groups.begin()->second.name();
    }
    // TODO Adjust group order
    // TODO, post event
}

void InputMethodManager::setGroupOrder(const std::vector<std::string> &groupOrder) {
    FCITX_D();
    d->groupOrder.clear();
    std::unordered_set<std::string> added;
    for (auto &groupName : groupOrder) {
        if (d->groups.count(groupName)) {
            d->groupOrder.push_back(groupName);
            added.insert(groupName);
        }
    }
    for (auto &p : d->groups) {
        if (!added.count(p.first)) {
            d->groupOrder.push_back(p.first);
        }
    }
    assert(d->groupOrder.size() == d->groups.size());
}

void InputMethodManager::save() {
    FCITX_D();
    InputMethodConfig config;
    std::vector<InputMethodGroupConfig> groups;
    config.currentGroup.setValue(d->currentGroup);
    config.groupOrder.setValue(std::vector<std::string>{d->groupOrder.begin(), d->groupOrder.end()});

    for (auto &p : d->groups) {
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
    d->instance = instance;
}
}
