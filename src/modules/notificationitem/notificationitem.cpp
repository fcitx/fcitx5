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

#include "notificationitem.h"
#include "dbusmenu.h"
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodentry.h"
#include <fmt/format.h>
#include <unistd.h>

#define NOTIFICATION_ITEM_DBUS_IFACE "org.kde.StatusNotifierItem"
#define NOTIFICATION_ITEM_DEFAULT_OBJ "/StatusNotifierItem"
#define NOTIFICATION_WATCHER_DBUS_ADDR "org.kde.StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_OBJ "/StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_IFACE "org.kde.StatusNotifierWatcher"

namespace fcitx {

class StatusNotifierItem : public dbus::ObjectVTable<StatusNotifierItem> {
public:
    StatusNotifierItem(NotificationItem *parent) : parent_(parent) {}

    void scroll(int delta, const std::string &_orientation) {
        std::string orientation = _orientation;
        std::transform(orientation.begin(), orientation.end(),
                       orientation.begin(), charutils::tolower);
        if (orientation != "vertical") {
            return;
        }
        deltaAcc_ += delta;
        while (deltaAcc_ >= 120) {
            parent_->instance()->enumerate(true);
            deltaAcc_ -= 120;
        }
        while (deltaAcc_ <= -120) {
            parent_->instance()->enumerate(false);
            deltaAcc_ += 120;
        }
    }
    void activate(int, int) { parent_->instance()->toggle(); }
    void secondaryActivate(int, int) {}
    std::string iconName() {
        if (auto ic = parent_->instance()->lastFocusedInputContext()) {
            if (auto entry = parent_->instance()->inputMethodEntry(ic)) {
                return entry->icon();
            }
        }
        return "input-keyboard";
    }

    dbus::DBusStruct<
        std::string,
        std::vector<dbus::DBusStruct<int32_t, int32_t, std::vector<uint8_t>>>,
        std::string, std::string>
    tooltip() {
        return {};
    }

    FCITX_OBJECT_VTABLE_METHOD(scroll, "Scroll", "is", "");
    FCITX_OBJECT_VTABLE_METHOD(activate, "Activate", "ii", "");
    FCITX_OBJECT_VTABLE_METHOD(secondaryActivate, "SecondaryActivate", "ii",
                               "");
    FCITX_OBJECT_VTABLE_SIGNAL(newIcon, "NewIcon", "");
    FCITX_OBJECT_VTABLE_SIGNAL(newToolTip, "NewToolTip", "");
    FCITX_OBJECT_VTABLE_SIGNAL(newIconThemePath, "NewIconThemePath", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(newAttentionIcon, "NewAttentionIcon", "");
    FCITX_OBJECT_VTABLE_SIGNAL(newStatus, "NewStatus", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(newTitle, "NewTitle", "");
    FCITX_OBJECT_VTABLE_SIGNAL(xayatanaNewLabel, "XAyatanaNewLabel", "ss");

    FCITX_OBJECT_VTABLE_PROPERTY(id, "Id", "s", [this]() { return "Fcitx"; });
    FCITX_OBJECT_VTABLE_PROPERTY(category, "Category", "s",
                                 [this]() { return "SystemServices"; });
    FCITX_OBJECT_VTABLE_PROPERTY(status, "Status", "s",
                                 [this]() { return "Active"; });
    FCITX_OBJECT_VTABLE_PROPERTY(iconName, "IconName", "s",
                                 [this]() { return iconName(); });
    FCITX_OBJECT_VTABLE_PROPERTY(attentionIconName, "AttentionIconName", "s",
                                 [this]() { return ""; });
    FCITX_OBJECT_VTABLE_PROPERTY(title, "Title", "s",
                                 [this]() { return _("Input Method"); });
    FCITX_OBJECT_VTABLE_PROPERTY(tooltip, "ToolTip", "(sa(iiay)ss)",
                                 [this]() { return tooltip(); });
    FCITX_OBJECT_VTABLE_PROPERTY(iconThemePath, "IconThemePath", "s",
                                 [this]() { return ""; });
    FCITX_OBJECT_VTABLE_PROPERTY(menu, "Menu", "o", [this]() {
        return dbus::ObjectPath("/MenuBar");
    });
    FCITX_OBJECT_VTABLE_PROPERTY(xayatanaLabel, "XAyatanaLabel", "s",
                                 [this]() { return ""; });
    FCITX_OBJECT_VTABLE_PROPERTY(XAyatanaLabelGuide, "XAyatanaLabelGuide", "s",
                                 [this]() { return ""; });
    FCITX_OBJECT_VTABLE_PROPERTY(xayatanaLabelOrderingIndex,
                                 "XAyatanaOrderingIndex", "u",
                                 [this]() { return 0; });

private:
    NotificationItem *parent_;
    int deltaAcc_ = 0;
};

NotificationItem::NotificationItem(Instance *instance)
    : instance_(instance), bus_(bus()),
      watcher_(std::make_unique<dbus::ServiceWatcher>(*bus_)),
      sni_(std::make_unique<StatusNotifierItem>(this)),
      menu_(std::make_unique<DBusMenu>(this)) {
    bus_->addObjectVTable(NOTIFICATION_ITEM_DEFAULT_OBJ,
                          NOTIFICATION_ITEM_DBUS_IFACE, *sni_);
    watcherEntry_.reset(watcher_->watchService(
        NOTIFICATION_WATCHER_DBUS_ADDR,
        [this](const std::string &, const std::string &,
               const std::string &newName) { setSerivceName(newName); }));

    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusIn, EventWatcherPhase::Default,
        [this](Event &) { sni_->newIcon(); }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextSwitchInputMethod, EventWatcherPhase::Default,
        [this](Event &) { sni_->newIcon(); }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) { sni_->newIcon(); }));
}

NotificationItem::~NotificationItem() = default;

dbus::Bus *NotificationItem::bus() { return dbus()->call<IDBusModule::bus>(); }

void NotificationItem::setSerivceName(const std::string &newName) {
    sniWatcherName_ = newName;
    // It's a new service anyway, set unregistered.
    setRegistered(false);
    registerSNI();
}

void NotificationItem::setRegistered(bool registered) {
    if (registered_ != registered) {
        registered_ = registered;

        for (auto &handler : handlers_.view()) {
            handler(registered_);
        }
    }
}

void NotificationItem::registerSNI() {
    if (!enabled_ || sniWatcherName_.empty()) {
        return;
    }
    auto call = bus_->createMethodCall(
        sniWatcherName_.c_str(), NOTIFICATION_WATCHER_DBUS_OBJ,
        NOTIFICATION_WATCHER_DBUS_IFACE, "RegisterStatusNotifierItem");
    call << serviceName_;
    pendingRegisterCall_.reset(call.callAsync(0, [this](dbus::Message msg) {
        setRegistered(!msg.isError());
        pendingRegisterCall_.reset();
        return true;
    }));
}

void NotificationItem::enable() {
    if (enabled_) {
        return;
    }

    serviceName_ =
        fmt::format("org.kde.StatusNotifierItem-{0}-{1}", getpid(), ++index_);
    if (!bus_->requestName(serviceName_, Flags<dbus::RequestNameFlag>(0))) {
        return;
    }
    enabled_ = true;
    registerSNI();
}

void NotificationItem::disable() {
    if (!enabled_) {
        return;
    }

    bus_->releaseName(serviceName_);

    enabled_ = false;
}

HandlerTableEntry<NotificationItemCallback> *
NotificationItem::watch(NotificationItemCallback callback) {
    return handlers_.add(callback);
}

class NotificationItemFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new NotificationItem(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::NotificationItemFactory)
