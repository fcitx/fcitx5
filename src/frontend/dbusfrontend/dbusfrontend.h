/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_
#define _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_

#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"

namespace fcitx {

class InputMethod1;

class DBusFrontendModule : public AddonInstance {
public:
    DBusFrontendModule(Instance *instance);
    ~DBusFrontendModule();

    dbus::Bus *bus();
    Instance *instance() { return instance_; }
    int nextIcIdx() { return ++icIdx_; }

private:
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());

    Instance *instance_;
    std::unique_ptr<dbus::Bus> portalBus_;
    std::unique_ptr<InputMethod1> inputMethod1_;
    std::unique_ptr<InputMethod1> inputMethod1Compatible_;
    std::unique_ptr<InputMethod1> portalInputMethod1_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>> events_;
    int icIdx_ = 0;
};
} // namespace fcitx

#endif // _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_
