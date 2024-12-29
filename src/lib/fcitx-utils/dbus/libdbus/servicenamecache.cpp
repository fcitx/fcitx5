/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "servicenamecache.h"
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include "../servicewatcher.h"
#include "bus_p.h"

fcitx::dbus::ServiceNameCache::ServiceNameCache(fcitx::dbus::Bus &bus)
    : watcher_(std::make_unique<ServiceWatcher>(bus)) {}

fcitx::dbus::ServiceNameCache::~ServiceNameCache() = default;

std::string fcitx::dbus::ServiceNameCache::owner(const std::string &query) {
    auto iter = nameMap_.find(query);
    if (iter == nameMap_.end()) {
        return {};
    }
    return iter->second;
}

void fcitx::dbus::ServiceNameCache::addWatch(const std::string &name) {
    auto iter = watcherMap_.find(name);
    if (watcherMap_.find(name) != watcherMap_.end()) {
        ++iter->second.first;
        FCITX_LIBDBUS_DEBUG() << "increase ref for " << name;
        return;
    }
    auto result = watcherMap_.emplace(
        name, std::make_pair(1, watcher_->watchService(
                                    name, [this](const std::string &service,
                                                 const std::string &,
                                                 const std::string &newName) {
                                        if (newName.empty()) {
                                            nameMap_.erase(newName);
                                        } else {
                                            nameMap_[service] = newName;
                                        }
                                    })));
    assert(result.second);
    FCITX_LIBDBUS_DEBUG() << "add Service name cache for " << name;
}

void fcitx::dbus::ServiceNameCache::removeWatch(const std::string &name) {
    auto iter = watcherMap_.find(name);
    if (iter != watcherMap_.end()) {
        FCITX_LIBDBUS_DEBUG() << "decrease ref for " << name;
        --iter->second.first;
        if (iter->second.first == 0) {
            watcherMap_.erase(iter);
            FCITX_LIBDBUS_DEBUG() << "remove service name cache for " << name;
        }
    }
}
