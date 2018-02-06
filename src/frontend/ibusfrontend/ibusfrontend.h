//
// Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_FRONTEND_IBUSFRONTEND_IBUSFRONTEND_H_
#define _FCITX_FRONTEND_IBUSFRONTEND_IBUSFRONTEND_H_

#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/event.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include <unistd.h>

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
