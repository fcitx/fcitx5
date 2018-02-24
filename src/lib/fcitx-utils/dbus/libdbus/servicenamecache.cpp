//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "servicenamecache.h"
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
