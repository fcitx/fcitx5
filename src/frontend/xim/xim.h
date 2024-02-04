/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_FRONTEND_XIM_XIM_H_
#define _FCITX_FRONTEND_XIM_XIM_H_

#include <unordered_map>
#include <xcb-imdkit/imdkit.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "xcb_public.h"

namespace fcitx {

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
    std::unique_ptr<HandlerTableEntry<EventHandler>> updateRootStyleCallback_;
    XIMConfig config_;
};
} // namespace fcitx

#endif // _FCITX_FRONTEND_XIM_XIM_H_
