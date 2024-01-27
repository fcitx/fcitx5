/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FRONTEND_IBUSFRONTEND_IBUSFRONTEND_H_
#define _FCITX_FRONTEND_IBUSFRONTEND_IBUSFRONTEND_H_

#include <unistd.h>
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"

namespace fcitx {

class IBusFrontend;

class IBusFrontendModule : public AddonInstance {
public:
    IBusFrontendModule(Instance *instance);
    ~IBusFrontendModule() override;

    dbus::Bus *bus();
    Instance *instance() { return instance_; }

private:
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());

    void replaceIBus(bool recheck);
    // Write socket file, call ensureIsIBus after a delay.
    void becomeIBus(bool recheck);
    // Check if org.freedesktop.IBus is owned by us and socket file is ours.
    void ensureIsIBus();

    const StandardPath &standardPath_ = StandardPath::global();
    Instance *instance_;
    std::unique_ptr<dbus::Bus> portalBus_;
    std::unique_ptr<IBusFrontend> inputMethod1_;
    std::unique_ptr<IBusFrontend> portalIBusFrontend_;
    std::unique_ptr<EventSourceTime> timeEvent_;

    std::set<std::string> socketPaths_;
    std::string addressWrote_;
    pid_t pidWrote_;
    int retry_ = 5;
};
} // namespace fcitx

#endif // _FCITX_FRONTEND_DBUSFRONTEND_DBUSFRONTEND_H_
