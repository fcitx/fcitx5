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
    if (watcherMap_.find(name) != watcherMap_.end()) {
        return;
    }
    watcherMap_.emplace(
        name,
        watcher_->watchService(name, [this](const std::string &service,
                                            const std::string &,
                                            const std::string &newName) {
            if (newName.empty()) {
                nameMap_.erase(newName);
            } else {
                nameMap_[service] = newName;
            }
        }));
}

void fcitx::dbus::ServiceNameCache::removeWatch(const std::string &name) {
    watcherMap_.erase(name);
}
