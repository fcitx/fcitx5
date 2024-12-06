/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_H_
#define _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_H_

#include <unordered_set>
#include <utility>
#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addoninstance.h"
#include "fcitx/instance.h"
#include "notifications_public.h"

namespace fcitx::notifications {

FCITX_CONFIGURATION(NotificationsConfig,
                    fcitx::Option<std::vector<std::string>> hiddenNotifications{
                        this, "HiddenNotifications",
                        _("Hidden Notifications")};);

struct NotificationItem {
    NotificationItem(uint64_t internalId,
                     NotificationActionCallback actionCallback,
                     NotificationClosedCallback closedCallback)
        : internalId_(internalId), actionCallback_(std::move(actionCallback)),
          closedCallback_(std::move(closedCallback)) {}
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

} // namespace fcitx::notifications

#endif // _FCITX_MODULES_NOTIFICATIONS_NOTIFICATIONS_H_
