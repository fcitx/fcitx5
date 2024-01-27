/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 * SPDX-FileCopyrightText: 2012-2013 Yichao Yu <yyc1992@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "notifications.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/icontheme.h"
#include "dbus_public.h"

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
                FCITX_DEBUG()
                    << "Notification ActionInvoked: " << id << " " << key;
                auto *item = findByGlobalId(id);
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
                auto *item = findByGlobalId(id);
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
    auto *item = find(internalId);
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

    message << appName << replaceId << IconTheme::iconName(appIcon) << summary
            << body;
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
        message.callAsync(0, [this, internalId](dbus::Message &message) {
            auto *item = find(internalId);
            if (!item) {
                return true;
            }
            if (message.isError()) {
                removeItem(*item);
                return true;
            }
            uint32_t globalId;
            if (!(message >> globalId)) {
                return true;
            }
            item->globalId_ = globalId;
            globalToInternalId_[globalId] = internalId;
            item->slot_.reset();
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
                FCITX_DEBUG() << "Dont show clicked: " << tipId;
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
