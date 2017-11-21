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
#ifndef _FCITX_MODULES_NOTIFICATIONITEM_DBUSMENU_H_
#define _FCITX_MODULES_NOTIFICATIONITEM_DBUSMENU_H_

#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx/inputcontext.h"
#include <unordered_set>

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
    dbus::Variant getProperty(int32_t, const std::string &) {
        // TODO implement this, document said this only for debug so we ignore
        // it for now
        throw dbus::MethodCallError("org.freedesktop.DBus.Error.NotSupported",
                                    "NotSupported");
    }
    std::tuple<uint32_t, DBusMenuLayout>
    getLayout(int parentId, int recursionDepth,
              const std::vector<std::string> &propertyNames) {
        std::tuple<uint32_t, DBusMenuLayout> result;
        static_assert(
            std::is_same<
                dbus::DBusSignatureToType<'u', '(', 'i', 'a', '{', 's', 'v',
                                          '}', 'a', 'v', ')'>::type,
                decltype(result)>::value,
            "Type not same as signature.");

        std::get<0>(result) = revision_;
        std::unordered_set<std::string> properties(propertyNames.begin(),
                                                   propertyNames.end());
        fillLayoutItem(parentId, recursionDepth, properties,
                       std::get<1>(result));
        return result;
    }

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
                                 [this]() { return version_; });
    FCITX_OBJECT_VTABLE_PROPERTY(status, "Status", "s",
                                 [this]() { return "normal"; });
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
    uint32_t revision_ = 0;
    NotificationItem *parent_;
    std::unique_ptr<EventSourceTime> timeEvent_;
    TrackableObjectReference<InputContext> lastRelevantIc_;
};

} // namespace fcitx

#endif // _FCITX_MODULES_NOTIFICATIONITEM_DBUSMENU_H_
