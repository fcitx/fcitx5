/*
 * SPDX-FileCopyrightText: 2020-2021 Vifly <viflythink@gmail.com>
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_FRONTEND_FCITX4FRONTEND_FCITX4FRONTEND_H_
#define _FCITX5_FRONTEND_FCITX4FRONTEND_FCITX4FRONTEND_H_

#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/event.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "config.h"

#ifdef ENABLE_X11
#include "xcb_public.h"
#endif

namespace fcitx {

class AddonInstance;
class Instance;
class Fcitx4InputMethod;

class Fcitx4FrontendModule : public AddonInstance {
public:
    Fcitx4FrontendModule(Instance *instance);

    dbus::Bus *bus();
    Instance *instance() { return instance_; }
    int nextIcIdx() { return ++icIdx_; }

    dbus::ServiceWatcher &serviceWatcher() { return *watcher_; }

    void addDisplay(const std::string &name);
    void removeDisplay(const std::string &name);

private:
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());
#ifdef ENABLE_X11
    FCITX_ADDON_DEPENDENCY_LOADER(xcb, instance_->addonManager());
#endif

    Instance *instance_;
    // A display number to dbus object mapping.
    std::unordered_map<int, std::unique_ptr<Fcitx4InputMethod>>
        fcitx4InputMethod_;
#ifdef ENABLE_X11
    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>> createdCallback_;
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>> closedCallback_;
#endif

    // Use multi handler table as reference counter like display number ->
    // display name mapping.
    MultiHandlerTable<int, std::string> table_;
    std::unordered_map<std::string,
                       std::unique_ptr<HandlerTableEntry<std::string>>>
        displayToHandle_;

    std::unique_ptr<HandlerTableEntry<EventHandler>> event_;
    int icIdx_ = 0;
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
};
} // namespace fcitx

#endif // _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_
