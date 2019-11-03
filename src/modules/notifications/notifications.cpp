//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
// Copyright (C) 2012~2013 by Yichao Yu
// yyc1992@gmail.com
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

#include "notifications.h"
#include "dbus_public.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include <fcntl.h>

#ifndef DBUS_TIMEOUT_USE_DEFAULT
#define DBUS_TIMEOUT_USE_DEFAULT (-1)
#endif

#define NOTIFICATIONS_SERVICE_NAME "org.freedesktop.Notifications"
#define NOTIFICATIONS_INTERFACE_NAME "org.freedesktop.Notifications"
#define NOTIFICATIONS_PATH "/org/freedesktop/Notifications"

namespace fcitx {

Notifications::Notifications(Instance *instance)
    : instance_(instance), dbus_(instance_->addonManager().addon("dbus")),
      bus_(dbus_->call<IDBusModule::bus>()), watcher_(*bus_) {
    reloadConfig();
    actionMatch_ = bus_->addMatch(
        dbus::MatchRule(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH,
                        NOTIFICATIONS_INTERFACE_NAME, "ActionInvoked"),
        [this](dbus::Message &message) {
            uint32_t id = 0;
            std::string key;
            if (message >> id >> key) {
                auto item = findByGlobalId(id);
                if (item && item->actionCallback_) {
                    item->actionCallback_(key);
                }
            }
            return true;
        });
    closedMatch_ = bus_->addMatch(
        dbus::MatchRule(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH,
                        NOTIFICATIONS_INTERFACE_NAME, "NotificationClosed"),
        [this](dbus::Message &message) {
            uint32_t id = 0;
            uint32_t reason = 0;
            if (message >> id >> reason) {
                auto item = findByGlobalId(id);
                if (item) {
                    if (item->closedCallback_) {
                        item->closedCallback_(reason);
                    }
                    removeItem(*item);
                }
            }
            return true;
        });
    watcherEntry_ = watcher_.watchService(
        NOTIFICATIONS_SERVICE_NAME,
        [this](const std::string &, const std::string &oldOwner,
               const std::string &newOwner) {
            if (!oldOwner.empty()) {
                capabilities_ = 0;
                call_.reset();
                items_.clear();
                globalToInternalId_.clear();
                internalId_ = epoch_ << 32u;
                epoch_++;
            }
            if (!newOwner.empty()) {
                auto message = bus_->createMethodCall(
                    NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH,
                    NOTIFICATIONS_INTERFACE_NAME, "GetCapabilities");
                call_ = message.callAsync(0, [this](dbus::Message &reply) {
                    std::vector<std::string> capabilities;
                    reply >> capabilities;
                    for (auto &capability : capabilities) {
                        if (capability == "actions") {
                            capabilities_ |= NotificationsCapability::Actions;
                        } else if (capability == "body") {
                            capabilities_ |= NotificationsCapability::Body;
                        } else if (capability == "body-hyperlinks") {
                            capabilities_ |= NotificationsCapability::Link;
                        } else if (capability == "body-markup") {
                            capabilities_ |= NotificationsCapability::Markup;
                        }
                    }
                    return true;
                });
            }
        });
}

Notifications::~Notifications() {}

void Notifications::updateConfig() {
    hiddenNotifications_.clear();
    for (const auto &id : config_.hiddenNotifications.value()) {
        hiddenNotifications_.insert(id);
    }
}

void Notifications::reloadConfig() {
    readAsIni(config_, "conf/notifications.conf");

    updateConfig();
}

void Notifications::save() {
    std::vector<std::string> values_;
    for (const auto &id : hiddenNotifications_) {
        values_.push_back(id);
    }
    config_.hiddenNotifications.setValue(std::move(values_));

    safeSaveAsIni(config_, "conf/notifications.conf");
}

void Notifications::closeNotification(uint64_t internalId) {
    auto item = find(internalId);
    if (item) {
        if (item->globalId_) {
            auto message = bus_->createMethodCall(
                NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH,
                NOTIFICATIONS_INTERFACE_NAME, "CloseNotification");
            message << item->globalId_;
            message.send();
        }
        removeItem(*item);
    }
}

#define TIMEOUT_REAL_TIME (100)
#define TIMEOUT_ADD_TIME (TIMEOUT_REAL_TIME + 10)

uint32_t Notifications::sendNotification(
    const std::string &appName, uint32_t replaceId, const std::string &appIcon,
    const std::string &summary, const std::string &body,
    const std::vector<std::string> &actions, int32_t timeout,
    NotificationActionCallback actionCallback,
    NotificationClosedCallback closedCallback) {
    auto message =
        bus_->createMethodCall(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH,
                               NOTIFICATIONS_INTERFACE_NAME, "Notify");
    auto *replaceItem = find(replaceId);
    if (!replaceItem) {
        replaceId = 0;
    } else {
        replaceId = replaceItem->globalId_;
        removeItem(*replaceItem);
    }

    message << appName << replaceId << appIcon << summary << body;
    message << actions;
    message << dbus::Container(dbus::Container::Type::Array,
                               dbus::Signature("{sv}"));
    message << dbus::ContainerEnd();
    message << timeout;

    internalId_++;
    auto result = items_.emplace(
        std::piecewise_construct, std::forward_as_tuple(internalId_),
        std::forward_as_tuple(internalId_, actionCallback, closedCallback));
    if (!result.second) {
        return 0;
    }

    int internalId = internalId_;
    auto &item = result.first->second;
    item.slot_ =
        message.callAsync(TIMEOUT_REAL_TIME * 1000 / 2,
                          [this, internalId](dbus::Message &message) {
                              auto item = find(internalId);
                              if (item) {
                                  if (!message.isError()) {
                                      uint32_t globalId;
                                      if (message >> globalId) {
                                          ;
                                      }
                                      if (item) {
                                          item->globalId_ = globalId;
                                          globalToInternalId_[globalId] =
                                              internalId;
                                      }
                                      item->slot_.reset();
                                      return true;
                                  }
                                  removeItem(*item);
                              }
                              return true;
                          });

    return internalId;
}

void Notifications::showTip(const std::string &tipId,
                            const std::string &appName,
                            const std::string &appIcon,
                            const std::string &summary, const std::string &body,
                            int32_t timeout) {
    if (hiddenNotifications_.count(tipId)) {
        return;
    }
    std::vector<std::string> actions = {"dont-show", _("Do not show again")};
    if (!capabilities_.test(NotificationsCapability::Actions)) {
        actions.clear();
    }
    lastTipId_ = sendNotification(
        appName, lastTipId_, appIcon, summary, body, actions, timeout,
        [this, tipId](const std::string &action) {
            if (action == "dont-show") {
                if (hiddenNotifications_.insert(tipId).second) {
                    save();
                }
            }
        },
        {});
}

class NotificationsModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Notifications(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::NotificationsModuleFactory)
