/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "notificationitem.h"
#include <unistd.h>
#include <fmt/format.h>
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodengine.h"
#include "fcitx/inputmethodentry.h"
#include "dbusmenu.h"

#define NOTIFICATION_ITEM_DBUS_IFACE "org.kde.StatusNotifierItem"
#define NOTIFICATION_ITEM_DEFAULT_OBJ "/StatusNotifierItem"
#define NOTIFICATION_WATCHER_DBUS_ADDR "org.kde.StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_OBJ "/StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_IFACE "org.kde.StatusNotifierWatcher"

namespace fcitx {

namespace {
bool isKDE() {
    std::string_view desktop;
    auto *desktopEnv = getenv("XDG_CURRENT_DESKTOP");
    if (desktopEnv) {
        desktop = desktopEnv;
    }
    return (desktop == "KDE");
}
} // namespace

class StatusNotifierItem : public dbus::ObjectVTable<StatusNotifierItem> {
public:
    StatusNotifierItem(NotificationItem *parent) : parent_(parent) {
        FCITX_LOG_IF(Info, inFlatpak_) << "Running inside flatpak.";
    }

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
        static bool preferSymbolic = !isKDE();
        std::string icon;
        if (preferSymbolic) {
            icon = "input-keyboard-symbolic";
        } else {
            icon = "input-keyboard";
        }
        if (auto *ic = parent_->instance()->lastFocusedInputContext()) {
            icon = parent_->instance()->inputMethodIcon(ic);
        }
        if (icon == "input-keyboard" && preferSymbolic) {
            return "input-keyboard-symbolic";
        }
        return IconTheme::iconName(icon, inFlatpak_);
    }

    std::string label() {
        if (!parent_->config().showLabel.value()) {
            return "";
        }
        if (auto *ic = parent_->instance()->lastFocusedInputContext()) {
            if (const auto *entry = parent_->instance()->inputMethodEntry(ic)) {
                if (entry->isKeyboard() || entry->icon().empty()) {
                    return entry->label();
                }
            }
        }
        return "";
    }

    static dbus::DBusStruct<
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

    FCITX_OBJECT_VTABLE_PROPERTY(id, "Id", "s", []() { return "Fcitx"; });
    FCITX_OBJECT_VTABLE_PROPERTY(category, "Category", "s",
                                 []() { return "SystemServices"; });
    FCITX_OBJECT_VTABLE_PROPERTY(status, "Status", "s",
                                 []() { return "Active"; });
    FCITX_OBJECT_VTABLE_PROPERTY(iconName, "IconName", "s",
                                 [this]() { return iconName(); });
    FCITX_OBJECT_VTABLE_PROPERTY(attentionIconName, "AttentionIconName", "s",
                                 []() { return ""; });
    FCITX_OBJECT_VTABLE_PROPERTY(title, "Title", "s",
                                 []() { return _("Input Method"); });
    FCITX_OBJECT_VTABLE_PROPERTY(tooltip, "ToolTip", "(sa(iiay)ss)",
                                 []() { return tooltip(); });
    FCITX_OBJECT_VTABLE_PROPERTY(iconThemePath, "IconThemePath", "s",
                                 []() { return ""; });
    FCITX_OBJECT_VTABLE_PROPERTY(menu, "Menu", "o",
                                 []() { return dbus::ObjectPath("/MenuBar"); });
    FCITX_OBJECT_VTABLE_PROPERTY(xayatanaLabel, "XAyatanaLabel", "s",
                                 [this]() { return label(); });
    FCITX_OBJECT_VTABLE_PROPERTY(XAyatanaLabelGuide, "XAyatanaLabelGuide", "s",
                                 [this]() { return label(); });
    FCITX_OBJECT_VTABLE_PROPERTY(xayatanaLabelOrderingIndex,
                                 "XAyatanaOrderingIndex", "u",
                                 []() { return 0; });

private:
    NotificationItem *parent_;
    int deltaAcc_ = 0;
    const bool inFlatpak_ = fs::isreg("/.flatpak-info");
};

NotificationItem::NotificationItem(Instance *instance)
    : instance_(instance), bus_(bus()),
      watcher_(std::make_unique<dbus::ServiceWatcher>(*bus_)),
      sni_(std::make_unique<StatusNotifierItem>(this)),
      menu_(std::make_unique<DBusMenu>(this)) {
    reloadConfig();
    watcherEntry_ = watcher_->watchService(
        NOTIFICATION_WATCHER_DBUS_ADDR,
        [this](const std::string &, const std::string &,
               const std::string &newName) { setSerivceName(newName); });
}

NotificationItem::~NotificationItem() = default;

dbus::Bus *NotificationItem::bus() { return dbus()->call<IDBusModule::bus>(); }

void NotificationItem::reloadConfig() {
    readAsIni(config_, "conf/notificationitem.conf");
}

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
    pendingRegisterCall_ = call.callAsync(0, [this](dbus::Message &msg) {
        FCITX_DEBUG() << "SNI Register result: " << msg.signature();
        if (msg.signature() == "s") {
            std::string mesg;
            msg >> mesg;
            FCITX_DEBUG() << mesg;
        }
        setRegistered(!msg.isError());
        pendingRegisterCall_.reset();
        return true;
    });
}

void NotificationItem::enable() {
    if (enabled_) {
        return;
    }
    // Ensure we are released.
    sni_->releaseSlot();
    bus_->addObjectVTable(NOTIFICATION_ITEM_DEFAULT_OBJ,
                          NOTIFICATION_ITEM_DBUS_IFACE, *sni_);

    serviceName_ = fmt::format("org.fcitx.Fcitx5.StatusNotifierItem-{0}-{1}",
                               getpid(), ++index_);
    if (!bus_->requestName(serviceName_, Flags<dbus::RequestNameFlag>(0))) {
        return;
    }
    enabled_ = true;
    registerSNI();

    auto updateIcon = [this](Event &) { newIcon(); };
    for (auto type : {EventType::InputContextFocusIn,
                      EventType::InputContextSwitchInputMethod,
                      EventType::InputMethodGroupChanged}) {
        eventHandlers_.emplace_back(instance_->watchEvent(
            type, EventWatcherPhase::Default, updateIcon));
    }
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextUpdateUI, EventWatcherPhase::Default,
        [this](Event &event) {
            if (static_cast<InputContextUpdateUIEvent &>(event).component() ==
                UserInterfaceComponent::StatusArea) {
                newIcon();
            }
        }));
}

void NotificationItem::disable() {
    if (!enabled_) {
        return;
    }

    bus_->releaseName(serviceName_);
    sni_->releaseSlot();

    enabled_ = false;
    eventHandlers_.clear();
}

std::unique_ptr<HandlerTableEntry<NotificationItemCallback>>
NotificationItem::watch(NotificationItemCallback callback) {
    return handlers_.add(std::move(callback));
}

void NotificationItem::newIcon() {
    // Make sure we only call it when it is registered.
    if (!sni_->isRegistered()) {
        return;
    }
    sni_->newIcon();
    sni_->xayatanaNewLabel(sni_->label(), sni_->label());
}

class NotificationItemFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new NotificationItem(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::NotificationItemFactory)
