/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_H_
#define _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addoninstance.h"
#include "fcitx/instance.h"
#include "notifications_public.h"
#include <functional>
#include <unordered_set>

namespace fcitx {

FCITX_CONFIGURATION(NotificationsConfig,
                    fcitx::Option<std::vector<std::string>> hiddenNotifications{
                        this, "HiddenNotifications",
                        _("Hidden Notifications")};);

struct NotificationItem {
    NotificationItem(uint64_t internalId,
                     NotificationActionCallback actionCallback,
                     NotificationClosedCallback closedCallback)
        : internalId_(internalId), actionCallback_(actionCallback),
          closedCallback_(closedCallback) {}
    uint32_t globalId_ = 0;
    uint64_t internalId_;
    NotificationActionCallback actionCallback_;
    NotificationClosedCallback closedCallback_;
    std::unique_ptr<dbus::Slot> slot_;
};

class Notifications final : public AddonInstance {
public:
    Notifications(Instance *instance);
    ~Notifications();
    Instance *instance() { return instance_; }
    void reloadConfig() override;
    void save() override;

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/notifications.conf");
        updateConfig();
    }

    void updateConfig();

    uint32_t sendNotification(const std::string &appName, uint32_t replaceId,
                              const std::string &appIcon,
                              const std::string &summary,
                              const std::string &body,
                              const std::vector<std::string> &actions,
                              int32_t timeout,
                              NotificationActionCallback actionCallback,
                              NotificationClosedCallback closedCallback);
    void showTip(const std::string &tipId, const std::string &appName,
                 const std::string &appIcon, const std::string &summary,
                 const std::string &body, int32_t timeout);

    void closeNotification(uint64_t internalId);

private:
    FCITX_ADDON_EXPORT_FUNCTION(Notifications, sendNotification);
    FCITX_ADDON_EXPORT_FUNCTION(Notifications, showTip);
    FCITX_ADDON_EXPORT_FUNCTION(Notifications, closeNotification);
    NotificationItem *find(uint64_t internalId) {
        auto itemIter = items_.find(internalId);
        if (itemIter == items_.end()) {
            return nullptr;
        }
        return &itemIter->second;
    }
    NotificationItem *findByGlobalId(uint32_t global) {
        auto iter = globalToInternalId_.find(global);
        if (iter == globalToInternalId_.end()) {
            return nullptr;
        }
        return find(iter->second);
    }

    void removeItem(NotificationItem &item) {
        globalToInternalId_.erase(item.globalId_);
        items_.erase(item.internalId_);
    }

    NotificationsConfig config_;
    Instance *instance_;
    AddonInstance *dbus_;
    dbus::Bus *bus_;

    Flags<NotificationsCapability> capabilities_;
    std::unordered_set<std::string> hiddenNotifications_;
    std::unique_ptr<dbus::Slot> call_;
    std::unique_ptr<dbus::Slot> actionMatch_;
    std::unique_ptr<dbus::Slot> closedMatch_;
    dbus::ServiceWatcher watcher_;
    std::unique_ptr<dbus::ServiceWatcherEntry> watcherEntry_;

    int lastTipId_ = 0;
    uint64_t internalId_ = 0;
    uint64_t epoch_ = 0;

    std::unordered_map<uint64_t, NotificationItem> items_;
    std::unordered_map<uint32_t, uint64_t> globalToInternalId_;
};
}

#endif // _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_H_
