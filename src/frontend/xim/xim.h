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
#ifndef _FCITX_FRONTEND_XIM_XIM_H_
#define _FCITX_FRONTEND_XIM_XIM_H_

#include <xcb-imdkit/imdkit.h>

#include <list>
#include <unordered_map>
#include <vector>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "xcb_public.h"

namespace fcitx {

class XIMModule;
class XIMServer;

FCITX_CONFIGURATION(XIMConfig,
                    Option<bool> useOnTheSpot{
                        this, "UseOnTheSpot",
                        _("Use On The Spot Style (Needs restarting)"), false};);

class XIMModule : public AddonInstance {
public:
    XIMModule(Instance *instance);
    ~XIMModule();

    FCITX_ADDON_DEPENDENCY_LOADER(xcb, instance_->addonManager());
    Instance *instance() { return instance_; }
    const auto &config() { return config_; }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/xim.conf");
    }
    void reloadConfig() override;

private:
    Instance *instance_;
    std::unordered_map<std::string, std::unique_ptr<XIMServer>> servers_;
    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>> createdCallback_;
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>> closedCallback_;
    XIMConfig config_;
};
} // namespace fcitx

#endif // _FCITX_FRONTEND_XIM_XIM_H_
