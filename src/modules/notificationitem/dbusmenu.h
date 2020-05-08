/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_NOTIFICATIONITEM_DBUSMENU_H_
#define _FCITX_MODULES_NOTIFICATIONITEM_DBUSMENU_H_

#include <unordered_set>
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx/inputcontext.h"

namespace fcitx {

class NotificationItem;
struct EventSourceTime;

class DBusMenu : public dbus::ObjectVTable<DBusMenu> {
    using DBusMenuProperty = dbus::DictEntry<std::string, dbus::Variant>;
    using DBusMenuProperties = std::vector<DBusMenuProperty>;
    using DBusMenuLayout = dbus::DBusStruct<int32_t, DBusMenuProperties,
                                            std::vector<dbus::Variant>>;

public:
    DBusMenu(NotificationItem *item);
    ~DBusMenu();

private:
    void event(int32_t id, const std::string &type, const dbus::Variant &,
               uint32_t);
    dbus::Variant getProperty(int32_t, const std::string &);
    std::tuple<uint32_t, DBusMenuLayout>
    getLayout(int parentId, int recursionDepth,
              const std::vector<std::string> &propertyNames);

    void fillLayoutItem(int32_t id, int depth,
                        const std::unordered_set<std::string> &propertyNames,
                        DBusMenuLayout &layout);
    void appendSubItem(std::vector<dbus::Variant> &, int32_t id, int depth,
                       const std::unordered_set<std::string> &propertyNames);
    void handleEvent(int32_t id);
    void appendProperty(DBusMenuProperties &properties,
                        const std::unordered_set<std::string> &propertyNames,
                        const std::string &name, dbus::Variant variant);
    void
    fillLayoutProperties(int32_t id,
                         const std::unordered_set<std::string> &propertyNames,
                         DBusMenuProperties &properties);

    std::vector<dbus::DBusStruct<int32_t, DBusMenuProperties>>
    getGroupProperties(const std::vector<int32_t> &ids,
                       const std::vector<std::string> &propertyNames) {
        std::unordered_set<std::string> properties(propertyNames.begin(),
                                                   propertyNames.end());
        std::vector<dbus::DBusStruct<int32_t, DBusMenuProperties>> result;
        for (auto id : ids) {
            result.emplace_back();
            std::get<0>(result.back()) = id;
            fillLayoutProperties(id, properties, std::get<1>(result.back()));
        }
        return result;
    }
    bool aboutToShow(int32_t id);

    InputContext *lastRelevantIc();

    FCITX_OBJECT_VTABLE_PROPERTY(version, "Version", "u",
                                 []() { return version_; });
    FCITX_OBJECT_VTABLE_PROPERTY(status, "Status", "s",
                                 []() { return "normal"; });
    // We don't use this.
    FCITX_OBJECT_VTABLE_SIGNAL(itemsPropertiesUpdated, "ItemsPropertiesUpdated",
                               "a(ia{sv})a(ias)");
    FCITX_OBJECT_VTABLE_SIGNAL(layoutUpdated, "LayoutUpdated", "ui");
    // We don't use this.
    FCITX_OBJECT_VTABLE_SIGNAL(itemActivationRequested,
                               "ItemActivationRequested", "iu");
    FCITX_OBJECT_VTABLE_METHOD(event, "Event", "isvu", "");
    FCITX_OBJECT_VTABLE_METHOD(getProperty, "GetProperty", "is", "v");
    FCITX_OBJECT_VTABLE_METHOD(getLayout, "GetLayout", "iias", "u(ia{sv}av)");
    FCITX_OBJECT_VTABLE_METHOD(getGroupProperties, "GetGroupProperties", "aias",
                               "a(ia{sv})");
    FCITX_OBJECT_VTABLE_METHOD(aboutToShow, "AboutToShow", "i", "b");

    constexpr static uint32_t version_ = 2;
    constexpr static uint32_t revision_ = 2;
    NotificationItem *parent_;
    std::unique_ptr<EventSourceTime> timeEvent_;
    TrackableObjectReference<InputContext> lastRelevantIc_;
    std::unordered_set<int32_t> requestedMenus_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_NOTIFICATIONITEM_DBUSMENU_H_
