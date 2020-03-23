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
#include "dbusmenu.h"
#include "fcitx-utils/log.h"
#include "fcitx/action.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/menu.h"
#include "fcitx/userinterfacemanager.h"
#include "notificationitem.h"

namespace fcitx {

//
// libdbusmenu-gtk have a strange 30000 limitation, in order to leverage this,
// we need
// some more hack
//
// max bit -> 14bit
//
//

#define DBUS_MENU_IFACE "com.canonical.dbusmenu"

enum BuiltInIndex {
    BII_InputMethodGroup = 1,
    BII_InputMethod,
    BII_Separator1,
    BII_Separator2,
    BII_Configure,
    BII_Restart,
    BII_Exit,
    BII_NormalEnd = 99,
    BII_InputMethodStart = 100,
    BII_InputMethodEnd = 199,
    BII_InputMethodGroupStart = 200,
    BII_InputMethodGroupEnd = 299,
    BII_Last = 300,
};

constexpr static int builtInIds = BII_Last;

DBusMenu::DBusMenu(NotificationItem *item) : parent_(item) {
    item->bus()->addObjectVTable("/MenuBar", DBUS_MENU_IFACE, *this);
}

DBusMenu::~DBusMenu() = default;

void DBusMenu::event(int32_t id, const std::string &type, const dbus::Variant &,
                     uint32_t) {
    // If top level menu is closed, reset the ic info.
    if (id == 0 && type == "closed") {
        lastRelevantIc_.unwatch();
        requestedMenus_.clear();
    }

    if (type != "clicked") {
        return;
    }
    // Why we need to delay the event, because we want to make ic has focus.
    timeEvent_ = parent_->instance()->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 30000, 0,
        [this, id](EventSourceTime *, uint64_t) {
            handleEvent(id);
            timeEvent_.reset();
            return true;
        });
}

void DBusMenu::handleEvent(int32_t id) {
    auto &imManager = parent_->instance()->inputMethodManager();
    if (id <= BII_NormalEnd) {
        switch (id) {
        case BII_Configure:
            parent_->instance()->configure();
            break;
        case BII_Restart:
            parent_->instance()->restart();
            break;
        case BII_Exit:
            parent_->instance()->exit();
            break;
        }
    } else if (id >= BII_InputMethodStart && id <= BII_InputMethodEnd) {
        size_t idx = id - BII_InputMethodStart;
        const auto &list = imManager.currentGroup().inputMethodList();
        if (idx >= list.size()) {
            return;
        }
        auto entry = imManager.entry(list[idx].name());
        if (!entry) {
            return;
        }

        parent_->instance()->setCurrentInputMethod(entry->uniqueName());
    } else if (id >= BII_InputMethodGroupStart &&
               id <= BII_InputMethodGroupEnd) {
        size_t idx = id - BII_InputMethodGroupStart;
        const auto &list = imManager.groups();
        if (idx >= list.size()) {
            return;
        }
        imManager.setCurrentGroup(list[idx]);
    } else {
        // Remove prefix.
        id -= builtInIds;
        if (auto ic = lastRelevantIc()) {
            if (auto action = parent_->instance()
                                  ->userInterfaceManager()
                                  .lookupActionById(id)) {
                action->activate(ic);
            }
        }
    }
}

void DBusMenu::appendSubItem(
    std::vector<dbus::Variant> &subItems, int32_t id, int depth,
    const std::unordered_set<std::string> &propertyNames) {
    DBusMenuLayout subLayout;
    fillLayoutItem(id, depth - 1, propertyNames, subLayout);
    subItems.emplace_back(std::move(subLayout));
}
void DBusMenu::appendProperty(
    DBusMenuProperties &properties,
    const std::unordered_set<std::string> &propertyNames,
    const std::string &name, dbus::Variant variant) {
    if (propertyNames.size() && !propertyNames.count(name)) {
        return;
    }
    properties.emplace_back(name, std::move(variant));
}

void DBusMenu::fillLayoutItem(
    int32_t id, int depth, const std::unordered_set<std::string> &propertyNames,
    DBusMenuLayout &layout) {
    std::get<0>(layout) = id;
    fillLayoutProperties(id, propertyNames, std::get<1>(layout));
    auto &subLayoutItems = std::get<2>(layout);

    if (id < 0) {
        return;
    }
    /* for dbus menu, we have
     * root (0,0) -> Group -> not visible if only one group
     *            -> Input Method
     *            -> configure current.
     *            -> configure (0,2)
     *            -> restart (0,3)
     *            -> exit (0,4)
     */
    if (depth == 0) {
        return;
    }
    requestedMenus_.insert(id);
    auto &imManager = parent_->instance()->inputMethodManager();
    if (id == 0) {
        // Group
        if (imManager.groupCount()) {
            appendSubItem(subLayoutItems, BII_InputMethodGroup, depth,
                          propertyNames);
        }
        // Input Method
        appendSubItem(subLayoutItems, BII_InputMethod, depth, propertyNames);
        // Separator
        appendSubItem(subLayoutItems, BII_Separator1, depth, propertyNames);
        bool hasAction = false;
        if (auto ic = lastRelevantIc()) {
            auto &statusArea = ic->statusArea();
            for (auto action : statusArea.allActions()) {
                if (!action->id()) {
                    // Obviously it's not registered with ui manager.
                    continue;
                }
                appendSubItem(subLayoutItems, builtInIds + action->id(), depth,
                              propertyNames);
                hasAction = true;
            }
        }
        if (hasAction) {
            appendSubItem(subLayoutItems, BII_Separator2, depth, propertyNames);
        }
        appendSubItem(subLayoutItems, BII_Configure, depth, propertyNames);
        appendSubItem(subLayoutItems, BII_Restart, depth, propertyNames);
        appendSubItem(subLayoutItems, BII_Exit, depth, propertyNames);
    } else if (id == BII_InputMethodGroup) {
        int idx = BII_InputMethodGroupStart;
        for (const auto &group : imManager.groups()) {
            FCITX_UNUSED(group);
            appendSubItem(subLayoutItems, idx, depth, propertyNames);
            idx++;
        }
    } else if (id == BII_InputMethod) {
        int idx = BII_InputMethodStart;
        for (const auto &item : imManager.currentGroup().inputMethodList()) {
            FCITX_UNUSED(item);
            appendSubItem(subLayoutItems, idx, depth, propertyNames);
            idx++;
        }
    } else if (id > builtInIds) {
        id -= builtInIds;
        if (auto action =
                parent_->instance()->userInterfaceManager().lookupActionById(
                    id)) {
            if (auto menu = action->menu()) {
                for (auto menuAction : menu->actions()) {
                    if (!menuAction->id()) {
                        // Obviously it's not registered with ui manager.
                        continue;
                    }
                    appendSubItem(subLayoutItems, builtInIds + menuAction->id(),
                                  depth, propertyNames);
                }
            }
        }
    }
}

void DBusMenu::fillLayoutProperties(
    int32_t id, const std::unordered_set<std::string> &propertyNames,
    DBusMenuProperties &properties) {
    if (id < 0) {
        return;
    }
    /* index == 0 means it has a sub menu */
    auto &imManager = parent_->instance()->inputMethodManager();
    if (id == 0) {
        appendProperty(properties, propertyNames, "children-display",
                       dbus::Variant("submenu"));
    } else if (id <= BII_NormalEnd) {
        switch (id) {
        case BII_InputMethod:
            appendProperty(properties, propertyNames, "children-display",
                           dbus::Variant("submenu"));
            appendProperty(properties, propertyNames, "label",
                           dbus::Variant(_("Input Method")));
            break;
        case BII_InputMethodGroup:
            appendProperty(properties, propertyNames, "children-display",
                           dbus::Variant("submenu"));
            appendProperty(properties, propertyNames, "label",
                           dbus::Variant(_("Group")));
            break;
        case BII_Separator1:
        case BII_Separator2:
            appendProperty(properties, propertyNames, "type",
                           dbus::Variant("separator"));
            break;
        case BII_Configure:
            /* this icon sucks on KDE, why configure doesn't have "configure" */
            appendProperty(properties, propertyNames, "label",
                           dbus::Variant(_("Configure")));
#if 0
                properties.emplace_back("icon-name", dbus::Variant("preferences-system"));
#endif
            break;
        case BII_Restart:
            appendProperty(properties, propertyNames, "label",
                           dbus::Variant(_("Restart")));
            appendProperty(properties, propertyNames, "icon-name",
                           dbus::Variant("view-refresh"));
            break;
        case BII_Exit:
            appendProperty(properties, propertyNames, "label",
                           dbus::Variant(_("Exit")));
            appendProperty(properties, propertyNames, "icon-name",
                           dbus::Variant("application-exit"));
            break;
        }
    } else if (id >= BII_InputMethodStart && id <= BII_InputMethodEnd) {
        size_t idx = id - BII_InputMethodStart;
        const auto &list = imManager.currentGroup().inputMethodList();
        if (idx >= list.size()) {
            return;
        }
        auto entry = imManager.entry(list[idx].name());
        if (!entry) {
            return;
        }
        appendProperty(properties, propertyNames, "label",
                       dbus::Variant(entry->name()));
        if (!entry->icon().empty()) {
            appendProperty(properties, propertyNames, "icon-name",
                           dbus::Variant(entry->icon()));
        }

        if (auto ic = lastRelevantIc()) {
            appendProperty(properties, propertyNames, "toggle-type",
                           dbus::Variant("radio"));
            // We can use pointer comparision here.
            appendProperty(
                properties, propertyNames, "toggle-state",
                dbus::Variant(parent_->instance()->inputMethodEntry(ic) == entry
                                  ? 1
                                  : 0));
        }
    } else if (id >= BII_InputMethodGroupStart &&
               id <= BII_InputMethodGroupEnd) {
        size_t idx = id - BII_InputMethodGroupStart;
        const auto &list = imManager.groups();
        if (idx >= list.size()) {
            return;
        }
        appendProperty(properties, propertyNames, "label",
                       dbus::Variant(list[idx]));
        appendProperty(properties, propertyNames, "toggle-type",
                       dbus::Variant("radio"));
        appendProperty(
            properties, propertyNames, "toggle-state",
            dbus::Variant(imManager.currentGroup().name() == list[idx] ? 1
                                                                       : 0));
    } else {
        id -= builtInIds;
        if (auto ic = lastRelevantIc()) {
            if (auto action = parent_->instance()
                                  ->userInterfaceManager()
                                  .lookupActionById(id)) {
                appendProperty(properties, propertyNames, "label",
                               dbus::Variant(action->shortText(ic)));
                appendProperty(properties, propertyNames, "icon-name",
                               dbus::Variant(action->icon(ic)));
                if (action->isCheckable()) {
                    appendProperty(properties, propertyNames, "toggle-type",
                                   dbus::Variant("radio"));
                    bool checked = action->isChecked(ic);

                    appendProperty(properties, propertyNames, "toggle-state",
                                   dbus::Variant(checked ? 1 : 0));
                }
                if (action->menu()) {
                    appendProperty(properties, propertyNames,
                                   "children-display",
                                   dbus::Variant("submenu"));
                }
            }
        }
    }
}

dbus::Variant DBusMenu::getProperty(int32_t, const std::string &) {
    // TODO implement this, document said this only for debug so we ignore
    // it for now
    throw dbus::MethodCallError("org.freedesktop.DBus.Error.NotSupported",
                                "NotSupported");
}

std::tuple<uint32_t, DBusMenu::DBusMenuLayout>
DBusMenu::getLayout(int parentId, int recursionDepth,
                    const std::vector<std::string> &propertyNames) {
    std::tuple<uint32_t, DBusMenuLayout> result;
    static_assert(
        std::is_same<dbus::DBusSignatureToType<'u', '(', 'i', 'a', '{', 's',
                                               'v', '}', 'a', 'v', ')'>::type,
                     decltype(result)>::value,
        "Type not same as signature.");

    std::get<0>(result) = revision_;
    std::unordered_set<std::string> properties(propertyNames.begin(),
                                               propertyNames.end());
    fillLayoutItem(parentId, recursionDepth, properties, std::get<1>(result));
    return result;
}

InputContext *DBusMenu::lastRelevantIc() {
    if (auto ic = lastRelevantIc_.get()) {
        return ic;
    }
    return parent_->instance()->mostRecentInputContext();
}

bool DBusMenu::aboutToShow(int32_t id) {
    if (id == 0) {
        if (auto ic = parent_->instance()->mostRecentInputContext()) {
            lastRelevantIc_ = ic->watch();
        }
        requestedMenus_.clear();
        return true;
    }
    if (requestedMenus_.count(id)) {
        return false;
    }
    return true;
}

} // namespace fcitx
