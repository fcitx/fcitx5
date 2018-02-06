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
#ifndef _FCITX_MODULES_NOTIFICATIONITEM_NOTIFICATIONITEM_H_
#define _FCITX_MODULES_NOTIFICATIONITEM_NOTIFICATIONITEM_H_

#include "dbus_public.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx/addoninstance.h"
#include "fcitx/instance.h"
#include "notificationitem_public.h"
#include <fcitx/addonmanager.h>
#include <memory>

namespace fcitx {

class StatusNotifierItem;
class DBusMenu;

class NotificationItem : public AddonInstance {
public:
    NotificationItem(Instance *instance);
    ~NotificationItem();

    dbus::Bus *bus();
    Instance *instance() { return instance_; }

    void setSerivceName(const std::string &newName);
    void setRegistered(bool);
    void registerSNI();
    void enable();
    void disable();
    bool registered() { return registered_; }
    std::unique_ptr<HandlerTableEntry<NotificationItemCallback>>
        watch(NotificationItemCallback);

private:
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());
    FCITX_ADDON_EXPORT_FUNCTION(NotificationItem, enable);
    FCITX_ADDON_EXPORT_FUNCTION(NotificationItem, disable);
    FCITX_ADDON_EXPORT_FUNCTION(NotificationItem, watch);
    FCITX_ADDON_EXPORT_FUNCTION(NotificationItem, registered);
    Instance *instance_;
    dbus::Bus *bus_;
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
    std::unique_ptr<StatusNotifierItem> sni_;
    std::unique_ptr<DBusMenu> menu_;
    std::unique_ptr<dbus::ServiceWatcherEntry> watcherEntry_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;
    std::unique_ptr<dbus::Slot> pendingRegisterCall_;
    std::string sniWatcherName_;
    int index_ = 0;
    std::string serviceName_;
    bool enabled_ = false;
    bool registered_ = false;
    HandlerTable<NotificationItemCallback> handlers_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_NOTIFICATIONITEM_NOTIFICATIONITEM_H_
