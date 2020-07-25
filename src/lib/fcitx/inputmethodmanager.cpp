/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "inputmethodmanager.h"
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <list>
#include <unordered_map>
#include "fcitx-config/iniparser.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/standardpath.h"
#include "addonmanager.h"
#include "inputmethodconfig_p.h"
#include "inputmethodengine.h"
#include "instance.h"
#include "misc_p.h"

namespace fcitx {

class InputMethodManagerPrivate : QPtrHolder<InputMethodManager> {
public:
    InputMethodManagerPrivate(AddonManager *addonManager_,
                              InputMethodManager *q)
        : QPtrHolder(q), addonManager_(addonManager_) {}

    FCITX_DEFINE_SIGNAL_PRIVATE(InputMethodManager, CurrentGroupAboutToChange);
    FCITX_DEFINE_SIGNAL_PRIVATE(InputMethodManager, CurrentGroupChanged);

    AddonManager *addonManager_;
    std::list<std::string> groupOrder_;
    std::unordered_map<std::string, InputMethodGroup> groups_;
    std::unordered_map<std::string, InputMethodEntry> entries_;
    Instance *instance_ = nullptr;
    std::unique_ptr<HandlerTableEntry<EventHandler>> eventWatcher_;
};

bool checkEntry(const InputMethodEntry &entry,
                const std::unordered_set<std::string> &inputMethods) {
    return (entry.name().empty() || entry.uniqueName().empty() ||
            entry.addon().empty() || inputMethods.count(entry.addon()) == 0)
               ? false
               : true;
}

InputMethodManager::InputMethodManager(AddonManager *addonManager)
    : d_ptr(std::make_unique<InputMethodManagerPrivate>(addonManager, this)) {}

InputMethodManager::~InputMethodManager() {}

void InputMethodManager::load() {
    FCITX_D();
    emit<InputMethodManager::CurrentGroupAboutToChange>(
        d->groupOrder_.empty() ? "" : d->groupOrder_.front());

    auto inputMethods =
        d->addonManager_->addonNames(AddonCategory::InputMethod);
    auto &path = StandardPath::global();
    auto filesMap =
        path.multiOpenAll(StandardPath::Type::PkgData, "inputmethod", O_RDONLY,
                          filter::Suffix(".conf"));
    for (const auto &file : filesMap) {
        auto &files = file.second;
        RawConfig config;
        // reverse the order, so we end up parse user file at last.
        for (auto iter = files.rbegin(), end = files.rend(); iter != end;
             iter++) {
            auto fd = iter->fd();
            readFromIni(config, fd);
        }

        InputMethodInfo imInfo;
        imInfo.load(config);
        // Remove ".conf"
        auto name = file.first.substr(0, file.first.size() - 5);
        InputMethodEntry entry = toInputMethodEntry(name, imInfo);
        if (checkEntry(entry, inputMethods) &&
            stringutils::isConcatOf(file.first, entry.uniqueName(), ".conf") &&
            d->entries_.count(entry.uniqueName()) == 0) {
            d->entries_.emplace(std::string(entry.uniqueName()),
                                std::move(entry));
        }
    }
    for (const auto &addonName : inputMethods) {
        auto addonInfo = d->addonManager_->addonInfo(addonName);
        // on request input method should always provides entry with config file
        if (!addonInfo || addonInfo->onDemand()) {
            continue;
        }
        auto engine = static_cast<InputMethodEngine *>(
            d->addonManager_->addon(addonName));
        if (!engine) {
            FCITX_WARN() << "Failed to load input method addon: " << addonName;
            continue;
        }
        auto newEntries = engine->listInputMethods();
        FCITX_INFO() << "Found " << newEntries.size() << " input method(s) "
                     << "in addon " << addonName;
        for (auto &newEntry : newEntries) {
            // ok we can't let you register something werid.
            if (checkEntry(newEntry, inputMethods) &&
                newEntry.addon() == addonName &&
                d->entries_.count(newEntry.uniqueName()) == 0) {
                d->entries_.emplace(std::string(newEntry.uniqueName()),
                                    std::move(newEntry));
            }
        }
    }

    loadConfig();
    // groupOrder guarantee to be non-empty at this point.
    emit<InputMethodManager::CurrentGroupChanged>(d->groupOrder_.front());
}

void InputMethodManager::loadConfig() {
    FCITX_D();
    auto &path = StandardPath::global();
    auto file = path.open(StandardPath::Type::PkgConfig, "profile", O_RDONLY);
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
            if (groupConfig.name.value().empty() ||
                groupConfig.defaultLayout.value().empty()) {
                continue;
            }
            auto result =
                d->groups_.emplace(groupConfig.name.value(),
                                   InputMethodGroup(groupConfig.name.value()));
            tempOrder.push_back(groupConfig.name.value());
            auto &group = result.first->second;
            group.setDefaultLayout(groupConfig.defaultLayout.value());
            auto &items = groupConfig.items.value();
            for (auto &item : items) {
                if (!d->entries_.count(item.name.value())) {
                    FCITX_LOG(Warn) << "Group Item " << item.name.value()
                                    << " in group " << groupConfig.name.value()
                                    << " is not valid. Removed.";
                    continue;
                }
                group.inputMethodList().emplace_back(
                    std::move(InputMethodGroupItem(item.name.value())
                                  .setLayout(item.layout.value())));
            }

            if (!group.inputMethodList().size()) {
                FCITX_LOG(Warn) << "Group " << groupConfig.name.value()
                                << " is empty. Removed.";
                d->groups_.erase(groupConfig.name.value());
                continue;
            }
            group.setDefaultInputMethod(groupConfig.defaultInputMethod.value());
        }
    }

    if (d->groups_.size() == 0) {
        FCITX_INFO() << "No valid input method group in configuration. "
                     << "Building a default one";
        buildDefaultGroup();
    } else {
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
    group.inputMethodList().emplace_back(InputMethodGroupItem("keyboard-us"));
    group.setDefaultInputMethod("");
    group.setDefaultLayout("us");
    setGroupOrder({name});
}

std::vector<std::string> InputMethodManager::groups() const {
    FCITX_D();
    return {d->groupOrder_.begin(), d->groupOrder_.end()};
}

int InputMethodManager::groupCount() const {
    FCITX_D();
    return d->groups_.size();
}

void InputMethodManager::setCurrentGroup(const std::string &groupName) {
    FCITX_D();
    if (groupName == d->groupOrder_.front()) {
        return;
    }
    auto iter =
        std::find(d->groupOrder_.begin(), d->groupOrder_.end(), groupName);
    if (iter != d->groupOrder_.end()) {
        emit<InputMethodManager::CurrentGroupAboutToChange>(
            d->groupOrder_.front());
        d->groupOrder_.splice(d->groupOrder_.begin(), d->groupOrder_, iter);
        emit<InputMethodManager::CurrentGroupChanged>(groupName);
    }
}

const InputMethodGroup &InputMethodManager::currentGroup() const {
    FCITX_D();
    return d->groups_.find(d->groupOrder_.front())->second;
}

InputMethodGroup &InputMethodManager::currentGroup() {
    FCITX_D();
    return d->groups_.find(d->groupOrder_.front())->second;
}

const InputMethodGroup *
InputMethodManager::group(const std::string &name) const {
    FCITX_D();
    return findValue(d->groups_, name);
}

void InputMethodManager::setGroup(InputMethodGroup newGroup) {
    FCITX_D();
    auto group = findValue(d->groups_, newGroup.name());
    if (group) {
        bool isCurrent = (group == &currentGroup());
        if (isCurrent) {
            emit<InputMethodManager::CurrentGroupAboutToChange>(
                d->groupOrder_.front());
        }
        auto &list = newGroup.inputMethodList();
        auto iter = std::remove_if(list.begin(), list.end(),
                                   [d](const InputMethodGroupItem &item) {
                                       return !d->entries_.count(item.name());
                                   });
        list.erase(iter, list.end());
        newGroup.setDefaultInputMethod(newGroup.defaultInputMethod());
        *group = std::move(newGroup);
        if (isCurrent) {
            emit<InputMethodManager::CurrentGroupChanged>(
                d->groupOrder_.front());
        }
    }
}

void InputMethodManager::setGroupOrder(
    const std::vector<std::string> &groupOrder) {
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

void InputMethodManager::addEmptyGroup(const std::string &name) {
    if (group(name)) {
        return;
    }
    FCITX_D();
    InputMethodGroup newGroup(name);
    newGroup.setDefaultLayout(currentGroup().defaultLayout());
    if (newGroup.defaultLayout().empty()) {
        newGroup.setDefaultLayout("us");
    }
    d->groups_.emplace(name, std::move(newGroup));
    d->groupOrder_.push_back(name);
}

void InputMethodManager::removeGroup(const std::string &name) {
    if (groupCount() == 1) {
        return;
    }
    FCITX_D();
    bool isCurrent = d->groupOrder_.front() == name;
    auto iter = d->groups_.find(name);
    if (iter != d->groups_.end()) {
        if (isCurrent) {
            emit<InputMethodManager::CurrentGroupAboutToChange>(
                d->groupOrder_.front());
        }
        d->groups_.erase(iter);
        d->groupOrder_.remove(name);

        if (isCurrent) {
            emit<InputMethodManager::CurrentGroupChanged>(
                d->groupOrder_.front());
        }
    }
}

void InputMethodManager::save() {
    FCITX_D();
    InputMethodConfig config;
    std::vector<InputMethodGroupConfig> groups;
    config.groupOrder.setValue(
        std::vector<std::string>{d->groupOrder_.begin(), d->groupOrder_.end()});

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

    safeSaveAsIni(config, "profile");
}

const InputMethodEntry *
InputMethodManager::entry(const std::string &name) const {
    FCITX_D();
    return findValue(d->entries_, name);
}
bool InputMethodManager::foreachEntries(
    const std::function<bool(const InputMethodEntry &entry)> callback) {
    FCITX_D();
    for (auto &p : d->entries_) {
        if (!callback(p.second)) {
            return false;
        }
    }
    return true;
}
} // namespace fcitx
