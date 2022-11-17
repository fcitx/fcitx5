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
#include "fcitx-utils/endian_p.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodengine.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/misc_p.h"
#include "classicui_public.h"
#include "dbusmenu.h"

#define NOTIFICATION_ITEM_DBUS_IFACE "org.kde.StatusNotifierItem"
#define NOTIFICATION_ITEM_DEFAULT_OBJ "/StatusNotifierItem"
#define NOTIFICATION_WATCHER_DBUS_ADDR "org.kde.StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_OBJ "/StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_IFACE "org.kde.StatusNotifierWatcher"
#define DBUS_MENU_IFACE "com.canonical.dbusmenu"

FCITX_DEFINE_LOG_CATEGORY(notificationitem, "notificationitem");
#define SNI_DEBUG() FCITX_LOGC(::notificationitem, Debug)

namespace fcitx {

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
        if (auto *ic = parent_->instance()->mostRecentInputContext()) {
            icon = parent_->instance()->inputMethodIcon(ic);
        }
        if (icon == "input-keyboard" && preferSymbolic) {
            return "input-keyboard-symbolic";
        }
        return IconTheme::iconName(icon, inFlatpak_);
    }

    std::string label() { return ""; }

    static dbus::DBusStruct<
        std::string,
        std::vector<dbus::DBusStruct<int32_t, int32_t, std::vector<uint8_t>>>,
        std::string, std::string>
    tooltip() {
        return {};
    }

    bool preferTextIcon(const std::string &label, const std::string &icon) {
        auto classicui = parent_->classicui();
        return classicui && !label.empty() &&
               ((icon == "input-keyboard" &&
                 classicui->call<IClassicUI::showLayoutNameInIcon>() &&
                 hasTwoKeyboardInCurrentGroup(parent_->instance())) ||
                classicui->call<IClassicUI::preferTextIcon>());
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

    FCITX_OBJECT_VTABLE_PROPERTY(category, "Category", "s",
                                 []() { return "SystemServices"; });
    FCITX_OBJECT_VTABLE_PROPERTY(id, "Id", "s", []() { return "Fcitx"; });
    FCITX_OBJECT_VTABLE_PROPERTY(title, "Title", "s",
                                 []() { return _("Input Method"); });
    FCITX_OBJECT_VTABLE_PROPERTY(status, "Status", "s",
                                 []() { return "Active"; });
    FCITX_OBJECT_VTABLE_PROPERTY(windowId, "WindowId", "u", []() { return 0; });
    FCITX_OBJECT_VTABLE_PROPERTY(
        iconName, "IconName", "s", ([this]() {
            std::string label, icon;
            if (auto *ic = parent_->instance()->mostRecentInputContext()) {
                label = parent_->instance()->inputMethodLabel(ic);
                icon = parent_->instance()->inputMethodIcon(ic);
            }
            return preferTextIcon(label, icon) ? "" : iconName();
        }));
    FCITX_OBJECT_VTABLE_PROPERTY(
        iconPixmap, "IconPixmap", "a(iiay)", ([this]() {
            std::vector<dbus::DBusStruct<int, int, std::vector<uint8_t>>>
                result;

            std::string label, icon;
            if (auto *ic = parent_->instance()->mostRecentInputContext()) {
                label = parent_->instance()->inputMethodLabel(ic);
                icon = parent_->instance()->inputMethodIcon(ic);
            }
            auto classicui = parent_->classicui();
            if (preferTextIcon(label, icon)) {
                if (lastLabel_ == label) {
                    result = lastLabelIcon_;
                } else {
                    for (unsigned int size : {16, 22, 32, 48}) {
                        // swap to network byte order if we are little endian
                        auto data =
                            classicui->call<IClassicUI::labelIcon>(label, size);
                        if (isLittleEndian()) {
                            uint32_t *uintBuf =
                                reinterpret_cast<uint32_t *>(data.data());
                            for (size_t i = 0;
                                 i < data.size() / sizeof(uint32_t); ++i) {
                                *uintBuf = htobe32(*uintBuf);
                                ++uintBuf;
                            }
                        }
                        result.emplace_back(size, size, std::move(data));
                    }
                    lastLabel_ = label;
                    lastLabelIcon_ = result;
                }
            }
            return result;
        }));
    FCITX_OBJECT_VTABLE_PROPERTY(overlayIconName, "OverlayIconName", "s",
                                 ([]() { return ""; }));
    FCITX_OBJECT_VTABLE_PROPERTY(
        overlayIconPixmap, "OverlayIconPixmap", "a(iiay)", ([]() {
            return std::vector<
                dbus::DBusStruct<int, int, std::vector<uint8_t>>>{};
        }));
    FCITX_OBJECT_VTABLE_PROPERTY(attentionIconName, "AttentionIconName", "s",
                                 []() { return ""; });
    FCITX_OBJECT_VTABLE_PROPERTY(
        attentionIconPixmap, "AttentionIconPixmap", "a(iiay)", ([]() {
            return std::vector<
                dbus::DBusStruct<int, int, std::vector<uint8_t>>>{};
        }));
    FCITX_OBJECT_VTABLE_PROPERTY(attentionMovieName, "AttentionMovieName", "s",
                                 []() { return ""; });
    FCITX_OBJECT_VTABLE_PROPERTY(tooltip, "ToolTip", "(sa(iiay)ss)",
                                 []() { return tooltip(); });
    FCITX_OBJECT_VTABLE_PROPERTY(itemIsMenu, "ItemIsMenu", "b",
                                 []() { return false; });
    FCITX_OBJECT_VTABLE_PROPERTY(menu, "Menu", "o",
                                 []() { return dbus::ObjectPath("/MenuBar"); });
    FCITX_OBJECT_VTABLE_PROPERTY(iconThemePath, "IconThemePath", "s",
                                 []() { return ""; });
    FCITX_OBJECT_VTABLE_PROPERTY(xayatanaLabel, "XAyatanaLabel", "s",
                                 [this]() { return label(); });
    FCITX_OBJECT_VTABLE_PROPERTY(XAyatanaLabelGuide, "XAyatanaLabelGuide", "s",
                                 [this]() { return label(); });
    FCITX_OBJECT_VTABLE_PROPERTY(xayatanaLabelOrderingIndex,
                                 "XAyatanaOrderingIndex", "u",
                                 []() { return 0; });
    FCITX_OBJECT_VTABLE_PROPERTY(iconAccessibleDesc, "IconAccessibleDesc", "s",
                                 []() { return _("Input Method"); });

private:
    NotificationItem *parent_;
    int deltaAcc_ = 0;
    const bool inFlatpak_ = fs::isreg("/.flatpak-info");
    // Quick cache for the icon.
    std::string lastLabel_;
    std::vector<dbus::DBusStruct<int, int, std::vector<uint8_t>>>
        lastLabelIcon_;
};

NotificationItem::NotificationItem(Instance *instance)
    : instance_(instance),
      watcher_(std::make_unique<dbus::ServiceWatcher>(*globalBus())),
      sni_(std::make_unique<StatusNotifierItem>(this)),
      menu_(std::make_unique<DBusMenu>(this)) {
    reloadConfig();
    watcherEntry_ = watcher_->watchService(
        NOTIFICATION_WATCHER_DBUS_ADDR,
        [this](const std::string &, const std::string &,
               const std::string &newName) { setServiceName(newName); });
}

NotificationItem::~NotificationItem() = default;

dbus::Bus *NotificationItem::globalBus() {
    return dbus()->call<IDBusModule::bus>();
}

void NotificationItem::setServiceName(const std::string &newName) {
    SNI_DEBUG() << "Old SNI Name: " << sniWatcherName_
                << " New Name: " << newName;
    sniWatcherName_ = newName;
    // It's a new service anyway, set unregistered.
    setRegistered(false);
    SNI_DEBUG() << "Current SNI enabled: " << enabled_;
    maybeScheduleRegister();
}

void NotificationItem::setRegistered(bool registered) {
    // Always clean up if it's not registered.
    if (!registered) {
        cleanUp();
    }

    if (registered_ == registered) {
        return;
    }
    registered_ = registered;

    if (registered_) {
        auto updateIcon = [this](Event &) {
            menu_->updateMenu();
            newIcon();
        };
        for (auto type : {EventType::InputContextFocusIn,
                          EventType::InputContextSwitchInputMethod,
                          EventType::InputMethodGroupChanged}) {
            eventHandlers_.emplace_back(instance_->watchEvent(
                type, EventWatcherPhase::Default, updateIcon));
        }
        eventHandlers_.emplace_back(instance_->watchEvent(
            EventType::InputContextUpdateUI, EventWatcherPhase::Default,
            [this](Event &event) {
                if (static_cast<InputContextUpdateUIEvent &>(event)
                        .component() == UserInterfaceComponent::StatusArea) {
                    newIcon();
                    menu_->updateMenu();
                }
            }));
    } else {
        cleanUp();
    }

    for (auto &handler : handlers_.view()) {
        handler(registered_);
    }
}

void NotificationItem::registerSNI() {
    if (!enabled_ || sniWatcherName_.empty() || registered_) {
        return;
    }

    setRegistered(false);
    try {
        // Ensure we are released.
        privateBus_ = std::make_unique<dbus::Bus>(globalBus()->address());
    } catch (...) {
        setRegistered(false);
        return;
    }
    privateBus_->attachEventLoop(&instance_->eventLoop());
    // Add object before request name.
    privateBus_->addObjectVTable(NOTIFICATION_ITEM_DEFAULT_OBJ,
                                 NOTIFICATION_ITEM_DBUS_IFACE, *sni_);
    privateBus_->addObjectVTable("/MenuBar", DBUS_MENU_IFACE, *menu_);
    SNI_DEBUG() << "Current DBus Unique Name" << privateBus_->uniqueName();
    auto call = privateBus_->createMethodCall(
        sniWatcherName_.c_str(), NOTIFICATION_WATCHER_DBUS_OBJ,
        NOTIFICATION_WATCHER_DBUS_IFACE, "RegisterStatusNotifierItem");
    call << privateBus_->uniqueName();

    SNI_DEBUG() << "Register SNI with name: " << privateBus_->uniqueName();
    pendingRegisterCall_ = call.callAsync(0, [this](dbus::Message &msg) {
        SNI_DEBUG() << "SNI Register result: " << msg.signature();
        if (msg.signature() == "s") {
            std::string mesg;
            msg >> mesg;
            SNI_DEBUG() << mesg;
        }
        setRegistered(!msg.isError());

        pendingRegisterCall_.reset();
        return true;
    });
    if (privateBus_) {
        privateBus_->flush();
    }
}

void NotificationItem::maybeScheduleRegister() {
    if (!enabled_ || sniWatcherName_.empty() || registered_) {
        return;
    }
    // Try to avoid Race between close dbus and register.
    scheduleRegister_ = instance_->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 300000, 0,
        [this](EventSourceTime *, uint64_t) {
            registerSNI();
            return true;
        });
}

void NotificationItem::enable() {
    if (enabled_) {
        return;
    }

    enabled_ = true;
    SNI_DEBUG() << "Enable SNI";
    maybeScheduleRegister();
}

void NotificationItem::disable() {
    if (!enabled_) {
        return;
    }

    SNI_DEBUG() << "Disable SNI";
    enabled_ = false;
    setRegistered(false);
}

void NotificationItem::cleanUp() {
    pendingRegisterCall_.reset();
    sni_->releaseSlot();
    menu_->releaseSlot();
    privateBus_.reset();

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
