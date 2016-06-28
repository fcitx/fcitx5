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
    ServiceWatcherPrivate(Bus &bus_)
        : bus(&bus_),
          watcherMap(
              [this](const std::string &key) {
                  auto slot = bus->addMatch(MATCH_PREFIX + key + MATCH_SUFFIX, [this](Message msg) {
                      std::string name, oldOwner, newOwner;
                      msg >> name >> oldOwner >> newOwner;

                      auto view = watcherMap.view(name);
                      for (auto &entry : view) {
                          entry.handler()(name, oldOwner, newOwner);
                      }
                      return true;
                  });
                  slots.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(slot));
              },
              [this](const std::string &key) { slots.erase(key); }) {}

    Bus *bus;
    MultiHandlerTable<std::string, ServiceWatcherCallback> watcherMap;
    std::unordered_map<std::string, std::unique_ptr<Slot>> slots;
};

ServiceWatcher::ServiceWatcher(Bus &bus) : d_ptr(std::make_unique<ServiceWatcherPrivate>(bus)) {}

HandlerTableEntry<ServiceWatcherCallback> *ServiceWatcher::watchService(const std::string &name,
                                                                        ServiceWatcherCallback callback) {
    FCITX_D();
    return d->watcherMap.add(name, callback);
}

ServiceWatcher::~ServiceWatcher() {}
}
}
