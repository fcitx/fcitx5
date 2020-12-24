/*
 * SPDX-FileCopyrightText: 2020-2020 Vifly <viflythink@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_
#define _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_

#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/event.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"

namespace fcitx {

class AddonInstance;
class Instance;
class InputMethod2;

class Fcitx4FrontendModule : public AddonInstance {
public:
    Fcitx4FrontendModule(Instance *instance);
    ~Fcitx4FrontendModule();

    dbus::Bus *bus();
    Instance *instance() { return instance_; }
    int nextIcIdx() { return ++icIdx_; }

private:
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());

    Instance *instance_;
    std::unique_ptr<dbus::Bus> portalBus_;
    std::unique_ptr<InputMethod2> InputMethod2_;
    std::unique_ptr<InputMethod2> InputMethod2Compatible_;
    std::unique_ptr<InputMethod2> portalInputMethod2_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> event_;
    int icIdx_ = 0;
};
} // namespace fcitx

#endif // _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_
