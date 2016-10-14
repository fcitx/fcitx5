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

#include "servicewatcher.h"
#include <unordered_map>

#define MATCH_PREFIX                                                                                                   \
    "type='signal',"                                                                                                   \
    "sender='org.freedesktop.DBus',"                                                                                   \
    "path='/org/freedesktop/DBus',"                                                                                    \
    "interface='org.freedesktop.DBus',"                                                                                \
    "member='NameOwnerChanged',"                                                                                       \
    "arg0='"

#define MATCH_SUFFIX "'"

namespace fcitx {
namespace dbus {

class ServiceWatcherPrivate {
public:
    ServiceWatcherPrivate(Bus &bus)
        : bus_(&bus),
          watcherMap_(
              [this](const std::string &key) {
                  auto slot = bus_->addMatch(MATCH_PREFIX + key + MATCH_SUFFIX, [this](Message msg) {
                      std::string name, oldOwner, newOwner;
                      msg >> name >> oldOwner >> newOwner;

                      auto view = watcherMap_.view(name);
                      for (auto &entry : view) {
                          entry(name, oldOwner, newOwner);
                      }
                      return true;
                  });
                  slots_.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(slot));
              },
              [this](const std::string &key) { slots_.erase(key); }) {}

    Bus *bus_;
    MultiHandlerTable<std::string, ServiceWatcherCallback> watcherMap_;
    std::unordered_map<std::string, std::unique_ptr<Slot>> slots_;
};

ServiceWatcher::ServiceWatcher(Bus &bus) : d_ptr(std::make_unique<ServiceWatcherPrivate>(bus)) {}

HandlerTableEntry<ServiceWatcherCallback> *ServiceWatcher::watchService(const std::string &name,
                                                                        ServiceWatcherCallback callback) {
    FCITX_D();
    return d->watcherMap_.add(name, callback);
}

ServiceWatcher::~ServiceWatcher() {}
}
}
