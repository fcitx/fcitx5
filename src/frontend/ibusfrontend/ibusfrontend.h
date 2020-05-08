/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FRONTEND_IBUSFRONTEND_IBUSFRONTEND_H_
#define _FCITX_FRONTEND_IBUSFRONTEND_IBUSFRONTEND_H_

#include <unistd.h>
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
class IBusFrontend;

class IBusFrontendModule : public AddonInstance {
public:
    IBusFrontendModule(Instance *instance);
    ~IBusFrontendModule();

    dbus::Bus *bus();
    Instance *instance() { return instance_; }

private:
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());

    void replaceIBus();
    void becomeIBus();

    Instance *instance_;
    std::string oldAddress_;
    std::unique_ptr<dbus::Bus> portalBus_;
    std::unique_ptr<IBusFrontend> inputMethod1_;
    std::unique_ptr<IBusFrontend> portalIBusFrontend_;
    std::unique_ptr<EventSourceTime> timeEvent_;

    std::string socketPath_;
    std::string addressWrote_;
    pid_t pidWrote_;
};
} // namespace fcitx

#endif // _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_
