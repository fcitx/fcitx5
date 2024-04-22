/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "servicewatcher.h"
#include <unordered_map>
#include "../trackableobject.h"

namespace fcitx::dbus {

class ServiceWatcherPrivate : public TrackableObject<ServiceWatcherPrivate> {
public:
    ServiceWatcherPrivate(Bus &bus)
        : bus_(&bus),
          watcherMap_(
              [this](const std::string &key) {
                  auto slot = bus_->addMatch(
                      MatchRule("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                "org.freedesktop.DBus", "NameOwnerChanged",
                                {key}),
                      [this](Message &msg) {
                          std::string name, oldOwner, newOwner;
                          msg >> name >> oldOwner >> newOwner;
                          querySlots_.erase(name);

                          auto view = watcherMap_.view(name);
                          for (auto &entry : view) {
                              entry(name, oldOwner, newOwner);
                          }
                          return false;
                      });
                  auto querySlot = bus_->serviceOwnerAsync(
                      key, 0, [this, key](Message &msg) {
                          // Key itself may be gone later, put it on the stack.
                          std::string pivotKey = key;
                          auto protector = watch();
                          std::string newName;
                          if (msg.type() != dbus::MessageType::Error) {
                              msg >> newName;
                          } else {
                            if (msg.errorName() != "org.freedesktop.DBus.Error.NameHasNoOwner") {
                                return false;
                            }
                          }
                          for (auto &entry : watcherMap_.view(pivotKey)) {
                              entry(pivotKey, "", newName);
                          }
                          // "this" maybe deleted as well because it's a member
                          // in lambda.
                          if (auto *that = protector.get()) {
                              that->querySlots_.erase(pivotKey);
                          }
                          return false;
                      });
                  if (!slot || !querySlot) {
                      return false;
                  }
                  slots_.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(key),
                                 std::forward_as_tuple(std::move(slot)));
                  querySlots_.emplace(
                      std::piecewise_construct, std::forward_as_tuple(key),
                      std::forward_as_tuple(std::move(querySlot)));
                  return true;
              },
              [this](const std::string &key) {
                  slots_.erase(key);
                  querySlots_.erase(key);
              }) {}

    Bus *bus_;
    MultiHandlerTable<std::string, ServiceWatcherCallback> watcherMap_;
    std::unordered_map<std::string, std::unique_ptr<Slot>> slots_;
    std::unordered_map<std::string, std::unique_ptr<Slot>> querySlots_;
};

ServiceWatcher::ServiceWatcher(Bus &bus)
    : d_ptr(std::make_unique<ServiceWatcherPrivate>(bus)) {}

std::unique_ptr<HandlerTableEntry<ServiceWatcherCallback>>
ServiceWatcher::watchService(const std::string &name,
                             ServiceWatcherCallback callback) {
    FCITX_D();
    return d->watcherMap_.add(name, std::move(callback));
}

ServiceWatcher::~ServiceWatcher() {}
} // namespace fcitx::dbus
