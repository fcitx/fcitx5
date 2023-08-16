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

    void loadConfig(const std::function<void(InputMethodManager &)>
                        &buildDefaultGroupCallback);
    void buildDefaultGroup(const std::function<void(InputMethodManager &)>
                               &buildDefaultGroupCallback);
    void setGroupOrder(const std::vector<std::string> &groupOrder);

    // Read entries from inputmethod/*.conf
    void loadStaticEntries(const std::unordered_set<std::string> &addonNames);
    // Read entries from addon->listInputMethods();
    void loadDynamicEntries(const std::unordered_set<std::string> &addonNames);

    FCITX_DEFINE_SIGNAL_PRIVATE(InputMethodManager, CurrentGroupAboutToChange);
    FCITX_DEFINE_SIGNAL_PRIVATE(InputMethodManager, CurrentGroupChanged);
    FCITX_DEFINE_SIGNAL_PRIVATE(InputMethodManager, GroupAdded);
    FCITX_DEFINE_SIGNAL_PRIVATE(InputMethodManager, GroupRemoved);

    AddonManager *addonManager_;
    std::list<std::string> groupOrder_;
    bool buildingGroup_ = false;
    std::unordered_map<std::string, InputMethodGroup> groups_;
    std::unordered_map<std::string, InputMethodEntry> entries_;
    Instance *instance_ = nullptr;
    std::unique_ptr<HandlerTableEntry<EventHandler>> eventWatcher_;
    int64_t timestamp_ = 0;
};

bool checkEntry(const InputMethodEntry &entry,
                const std::unordered_set<std::string> &inputMethods) {
    return !(entry.name().empty() || entry.uniqueName().empty() ||
             entry.addon().empty() || inputMethods.count(entry.addon()) == 0);
}

void InputMethodManagerPrivate::loadConfig(
    const std::function<void(InputMethodManager &)>
        &buildDefaultGroupCallback) {
    const auto &path = StandardPath::global();
    auto file = path.open(StandardPath::Type::PkgConfig, "profile", O_RDONLY);
    RawConfig config;
    if (file.fd() >= 0) {
        readFromIni(config, file.fd());
    }
    InputMethodConfig imConfig;
    imConfig.load(config);

    groups_.clear();
    std::vector<std::string> tempOrder;
    if (!imConfig.groups.value().empty()) {
        const auto &groupsConfig = imConfig.groups.value();
        for (const auto &groupConfig : groupsConfig) {
            // group must have a name
            if (groupConfig.name.value().empty() ||
                groupConfig.defaultLayout.value().empty()) {
                continue;
            }
            auto result =
                groups_.emplace(groupConfig.name.value(),
                                InputMethodGroup(groupConfig.name.value()));
            tempOrder.push_back(groupConfig.name.value());
            auto &group = result.first->second;
            group.setDefaultLayout(groupConfig.defaultLayout.value());
            const auto &items = groupConfig.items.value();
            for (const auto &item : items) {
                if (!entries_.count(item.name.value())) {
                    FCITX_WARN() << "Group Item " << item.name.value()
                                 << " in group " << groupConfig.name.value()
                                 << " is not valid. Removed.";
                    continue;
                }
                group.inputMethodList().emplace_back(
                    std::move(InputMethodGroupItem(item.name.value())
                                  .setLayout(item.layout.value())));
            }

            if (group.inputMethodList().empty()) {
                FCITX_WARN() << "Group " << groupConfig.name.value()
                             << " is empty. Removed.";
                groups_.erase(groupConfig.name.value());
                continue;
            }
            group.setDefaultInputMethod(groupConfig.defaultInputMethod.value());
        }
    }

    if (groups_.empty()) {
        FCITX_INFO() << "No valid input method group in configuration. "
                     << "Building a default one";
        buildDefaultGroup(buildDefaultGroupCallback);
    } else {
        if (!imConfig.groupOrder.value().empty()) {
            setGroupOrder(imConfig.groupOrder.value());
        } else {
            setGroupOrder(tempOrder);
        }
    }
}

void InputMethodManagerPrivate::buildDefaultGroup(
    const std::function<void(InputMethodManager &)>
        &buildDefaultGroupCallback) {
    groups_.clear();
    if (buildDefaultGroupCallback) {
        buildingGroup_ = true;
        buildDefaultGroupCallback(*q_func());
        buildingGroup_ = false;
    } else {
        std::string name = _("Default");
        auto result = groups_.emplace(name, InputMethodGroup(name));
        auto &group = result.first->second;
        group.inputMethodList().emplace_back(
            InputMethodGroupItem("keyboard-us"));
        group.setDefaultInputMethod("");
        group.setDefaultLayout("us");
        setGroupOrder({name});
    }
    assert(!groups_.empty());
    assert(!groupOrder_.empty());
}

void InputMethodManagerPrivate::loadStaticEntries(
    const std::unordered_set<std::string> &addonNames) {
    const auto &path = StandardPath::global();
    timestamp_ = path.timestamp(StandardPath::Type::PkgData, "inputmethod");
    auto filesMap =
        path.multiOpenAll(StandardPath::Type::PkgData, "inputmethod", O_RDONLY,
                          filter::Suffix(".conf"));
    for (const auto &file : filesMap) {
        const auto name = file.first.substr(0, file.first.size() - 5);
        if (entries_.count(name) != 0) {
            continue;
        }
        const auto &files = file.second;
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
        InputMethodEntry entry = toInputMethodEntry(name, imInfo);
        if (checkEntry(entry, addonNames) &&
            stringutils::isConcatOf(file.first, entry.uniqueName(), ".conf")) {
            entries_.emplace(std::string(entry.uniqueName()), std::move(entry));
        }
    }
}

void InputMethodManagerPrivate::loadDynamicEntries(
    const std::unordered_set<std::string> &addonNames) {
    for (const auto &addonName : addonNames) {
        const auto *addonInfo = addonManager_->addonInfo(addonName);
        // on request input method should always provides entry with config file
        if (!addonInfo || addonInfo->onDemand()) {
            continue;
        }
        auto *engine =
            static_cast<InputMethodEngine *>(addonManager_->addon(addonName));
        if (!engine) {
            FCITX_WARN() << "Failed to load input method addon: " << addonName;
            continue;
        }
        auto newEntries = engine->listInputMethods();
        FCITX_INFO() << "Found " << newEntries.size() << " input method(s) "
                     << "in addon " << addonName;
        for (auto &newEntry : newEntries) {
            // ok we can't let you register something werid.
            if (checkEntry(newEntry, addonNames) &&
                newEntry.addon() == addonName &&
                entries_.count(newEntry.uniqueName()) == 0) {
                entries_.emplace(std::string(newEntry.uniqueName()),
                                 std::move(newEntry));
            }
        }
    }
}

InputMethodManager::InputMethodManager(AddonManager *addonManager)
    : d_ptr(std::make_unique<InputMethodManagerPrivate>(addonManager, this)) {}

InputMethodManager::~InputMethodManager() {}

void InputMethodManager::load(const std::function<void(InputMethodManager &)>
                                  &buildDefaultGroupCallback) {
    FCITX_D();
    emit<InputMethodManager::CurrentGroupAboutToChange>(
        d->groupOrder_.empty() ? "" : d->groupOrder_.front());

    auto addonNames = d->addonManager_->addonNames(AddonCategory::InputMethod);
    d->loadStaticEntries(addonNames);
    d->loadDynamicEntries(addonNames);

    d->loadConfig(buildDefaultGroupCallback);
    // groupOrder guarantee to be non-empty at this point.
    emit<InputMethodManager::CurrentGroupChanged>(d->groupOrder_.front());
}

void InputMethodManager::reset(const std::function<void(InputMethodManager &)>
                                   &buildDefaultGroupCallback) {
    FCITX_D();
    emit<InputMethodManager::CurrentGroupAboutToChange>(
        d->groupOrder_.empty() ? "" : d->groupOrder_.front());
    d->buildDefaultGroup(buildDefaultGroupCallback);
    // groupOrder guarantee to be non-empty at this point.
    emit<InputMethodManager::CurrentGroupChanged>(d->groupOrder_.front());
}

void InputMethodManager::refresh() {
    FCITX_D();
    auto addonNames = d->addonManager_->addonNames(AddonCategory::InputMethod);
    d->loadStaticEntries(addonNames);
    d->loadDynamicEntries(addonNames);
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

void InputMethodManager::enumerateGroupTo(const std::string &groupName) {
    FCITX_D();
    if (groupName == d->groupOrder_.front()) {
        return;
    }
    auto iter =
        std::find(d->groupOrder_.begin(), d->groupOrder_.end(), groupName);
    if (iter != d->groupOrder_.end()) {
        emit<InputMethodManager::CurrentGroupAboutToChange>(
            d->groupOrder_.front());
        d->groupOrder_.splice(d->groupOrder_.begin(), d->groupOrder_, iter,
                              d->groupOrder_.end());
        emit<InputMethodManager::CurrentGroupChanged>(groupName);
    }
}

const InputMethodGroup &InputMethodManager::currentGroup() const {
    FCITX_D();
    return d->groups_.find(d->groupOrder_.front())->second;
}

void InputMethodManager::enumerateGroup(bool forward) {
    FCITX_D();
    if (groupCount() < 2) {
        return;
    }
    emit<InputMethodManager::CurrentGroupAboutToChange>(d->groupOrder_.front());
    if (forward) {
        d->groupOrder_.splice(d->groupOrder_.end(), d->groupOrder_,
                              d->groupOrder_.begin());
    } else {
        d->groupOrder_.splice(d->groupOrder_.begin(), d->groupOrder_,
                              std::prev(d->groupOrder_.end()));
    }
    emit<InputMethodManager::CurrentGroupChanged>(d->groupOrder_.front());
}

void InputMethodManager::setDefaultInputMethod(const std::string &name) {
    FCITX_D();
    auto &currentGroup = d->groups_.find(d->groupOrder_.front())->second;
    currentGroup.setDefaultInputMethod(name);
}

const InputMethodGroup *
InputMethodManager::group(const std::string &name) const {
    FCITX_D();
    return findValue(d->groups_, name);
}

void InputMethodManager::setGroup(InputMethodGroup newGroupInfo) {
    FCITX_D();
    auto *group = findValue(d->groups_, newGroupInfo.name());
    if (!group) {
        return;
    }
    bool isCurrent = false;
    if (!d->buildingGroup_) {
        isCurrent = (group == &currentGroup());
        if (isCurrent) {
            emit<InputMethodManager::CurrentGroupAboutToChange>(
                d->groupOrder_.front());
        }
    }
    auto &list = newGroupInfo.inputMethodList();
    auto iter = std::remove_if(list.begin(), list.end(),
                               [d](const InputMethodGroupItem &item) {
                                   return !d->entries_.count(item.name());
                               });
    list.erase(iter, list.end());
    newGroupInfo.setDefaultInputMethod(newGroupInfo.defaultInputMethod());
    *group = std::move(newGroupInfo);
    if (!d->buildingGroup_ && isCurrent) {
        emit<InputMethodManager::CurrentGroupChanged>(d->groupOrder_.front());
    }
}

void InputMethodManagerPrivate::setGroupOrder(
    const std::vector<std::string> &groupOrder) {
    groupOrder_.clear();
    std::unordered_set<std::string> added;
    for (const auto &groupName : groupOrder) {
        if (groups_.count(groupName)) {
            groupOrder_.push_back(groupName);
            added.insert(groupName);
        }
    }
    for (auto &p : groups_) {
        if (!added.count(p.first)) {
            groupOrder_.push_back(p.first);
        }
    }
    assert(groupOrder_.size() == groups_.size());
}

void InputMethodManager::addEmptyGroup(const std::string &name) {
    if (group(name)) {
        return;
    }
    FCITX_D();
    InputMethodGroup newGroup(name);
    if (groupCount()) {
        newGroup.setDefaultLayout(currentGroup().defaultLayout());
    }
    if (newGroup.defaultLayout().empty()) {
        newGroup.setDefaultLayout("us");
    }
    d->groups_.emplace(name, std::move(newGroup));
    d->groupOrder_.push_back(name);
    if (!d->buildingGroup_) {
        emit<InputMethodManager::GroupAdded>(name);
    }
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
        if (!d->buildingGroup_) {
            emit<InputMethodManager::GroupRemoved>(name);
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
    const std::function<bool(const InputMethodEntry &entry)> &callback) {
    FCITX_D();
    for (auto &p : d->entries_) {
        if (!callback(p.second)) {
            return false;
        }
    }
    return true;
}

void InputMethodManager::setGroupOrder(const std::vector<std::string> &groups) {
    FCITX_D();
    if (!d->buildingGroup_) {
        throw std::runtime_error("Called not within building group");
    }
    d->setGroupOrder(groups);
}

bool InputMethodManager::checkUpdate() const {
    FCITX_D();
    auto timestamp = StandardPath::global().timestamp(
        StandardPath::Type::PkgData, "inputmethod");
    return timestamp > d->timestamp_;
}

} // namespace fcitx
